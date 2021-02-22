# PartCode
Job related partional code .

# oasis
此目录下的代码是基于A33 Android5.1系统下完成的。主要实现 ntp8230 和 npca110 两个芯片的字符设备驱动，同时在 framework中增加新的API接口，提供给应用APP。
代码中的 packages应用代码只是用来调试 framework中的API接口，并非实际项目APP。
hardware 是实现对 ntp8230 和 npca110 字符设备文件的访问，最终会编译成 hal so库文件。
external 主要是针对访问文件时的读、写等权限修改。

