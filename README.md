# dump
dump the dll loaded by libmono.so

一些unity3d游戏会加密c#编译出来的dll文件,然后通过修改后的libmono.so加载并解密,
目的是为了防止使用ILSpy之类的工具反编译dll.本工具的作用就是通过加载修改后的libmono.so,
然后加载加密的dll,可以一键dump出未经加密的dll.
