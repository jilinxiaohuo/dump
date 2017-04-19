#include <jni.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <android/log.h>
#include <dirent.h>
#include <errno.h>
#include "Dump.h"

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,"dump",__VA_ARGS__)

#define va_start(v,l)	__builtin_va_start(v,l)
#define va_end(v)	__builtin_va_end(v)

typedef enum {
	MONO_IMAGE_OK,
	MONO_IMAGE_ERROR_ERRNO,
	MONO_IMAGE_MISSING_ASSEMBLYREF,
	MONO_IMAGE_IMAGE_INVALID
} MonoImageOpenStatus;

typedef struct MonoImage {
	int   ref_count;
	void *raw_data_handle;
	char *raw_data;
}MonoImage;

typedef int gboolean;

void SetText(JNIEnv* env, jobject obj, const char* format, ...)
{
	va_list args;

	va_start(args, format);

	char out[65535] = {0};
	vsprintf(out, format, args);

	jclass native_class = env->GetObjectClass(obj);
	jmethodID id = env->GetMethodID(native_class, "SetText","(Ljava/lang/String;)V");
	jstring str = env->NewStringUTF(out);

	env->CallVoidMethod(obj, id, str);
	env->DeleteLocalRef(str);
}

bool Decode(const char* dir, const char* dllname, JNIEnv * env, jobject obj)
{

	typedef MonoImage* (*mono_image_open_from_data_with_name_t)(
			char* data,
			unsigned int data_len,
			gboolean need_copy,
			MonoImageOpenStatus* status,
			gboolean refonly,
			const char* name
	);

	char filemono[1024];
	sprintf(filemono, "%s/libmono.so", dir);
	// use your string
	void* mod = dlopen(filemono, RTLD_NOW);
	if(mod == NULL)
	{
		SetText(env, obj, "can not load libmono.so \n%s\n", dlerror());
		LOGD("can not load libmono.so %s", dlerror());
		return false;
	}
	else
	{
		mono_image_open_from_data_with_name_t mono_image_open_from_data_with_name;
		mono_image_open_from_data_with_name = (mono_image_open_from_data_with_name_t)dlsym(mod, "mono_image_open_from_data_with_name");

		if(mono_image_open_from_data_with_name == NULL)
		{
			SetText(env, obj, "can not get address of mono_image_open_from_data_with_name\n%s", dlerror());
			LOGD("can not find mono_image_open_from_data_with_name");
			return false;
		}
		else
		{
			char fileassembly[1024];
			sprintf(fileassembly, "%s/%s", dir, dllname);

			FILE* fp = fopen(fileassembly, "rb");
			if(fp == NULL)
			{
				SetText(env, obj, "can not open assembly file %s\n", fileassembly, strerror(errno));
				LOGD("can not open assembly file %s", dllname);
				return false;
			}
			else
			{
				fseek(fp, 0, SEEK_END);
				int size = ftell(fp);
				rewind(fp);

				LOGD("malloc %d", size);
				char* data = (char*)malloc(size);
				if(data == NULL)
				{
					SetText(env, obj, "can not alloc memory %d bytes\n", size);
					fclose(fp);
					return false;
				}

				fread(data, 1, size, fp);
				fclose(fp);

				MonoImageOpenStatus status;
				MonoImage* image = mono_image_open_from_data_with_name(data, size, 1, &status, 0, dllname);

				LOGD("mono_image_open_from_data_with_name end %p %p", image, image->raw_data);
				free(data);

				char outfile[1024] = {0};
				sprintf(outfile, "%s.decode", fileassembly);

				FILE* fpout = fopen(outfile, "wb");
				if(fpout == NULL)
				{
					SetText(env, obj, "can not create file %s\n", outfile);
					LOGD("can not create file %s", outfile);
					return false;
				}
				else
				{
					int write = fwrite(image->raw_data, 1, size, fpout);

					fclose(fpout);

					SetText(env, obj, "output decode data to %s. %d bytes\n\n", outfile, write);
					LOGD("create file %s  %d/%d", outfile, write, size);
				}
			}
		}
		dlclose(mod);
	}

	return true;
}

void SearchDir(const char* root, const char *path,JNIEnv * env, jobject obj)
{
	LOGD("%s %s",root, path);
	DIR *dp;
	struct dirent *dmsg;
	char addpath[1024] = {0};

	if ((dp = opendir(path)) != NULL)
	{
		while ((dmsg = readdir(dp)) != NULL)
		{
			if (!strcmp(dmsg->d_name, ".") || !strcmp(dmsg->d_name, ".."))
				continue;


			strcpy(addpath, path);
			strcat(addpath, "/");
			strcat(addpath, dmsg->d_name);

			if (dmsg->d_type == DT_DIR )
			{
				if(strchr(dmsg->d_name, '.'))
				{
					if((strcmp(strchr(dmsg->d_name, '.'), dmsg->d_name)==0))
					{
						continue;
					}
				}
				SearchDir(root, addpath, env, obj);
			}
			else
			{
				int len = strlen(dmsg->d_name);
				if(len > 4)
				{
					if(strstr(&dmsg->d_name[len - 4], ".dll") )
					{
						if(false == Decode(root, dmsg->d_name, env, obj))
						{
							return;
						}
					}
				}
			}
		}

		closedir(dp);
	}
	else
	{
		SetText(env, obj, "can not open dir %s\n%s", path, strerror(errno));
		LOGD("can not open dir %s", path);
	}

}


JNIEXPORT void JNICALL Java_com_example_dump_DumpJni_dump
(JNIEnv * env, jobject obj, jstring root)
{

	const char *dir = env->GetStringUTFChars(root, 0);

	SearchDir(dir, dir, env, obj);

	env->ReleaseStringUTFChars(root, dir);

}
