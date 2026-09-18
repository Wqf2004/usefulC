# 注意事项

当前dfm/src/file_op.c中的 `file_delete_line` 无法删除换行，只能删除这一行里的内容，故待操作的文件data/b.txt，初始化时除了说明行，还需要加一个换行。

读代码发现了之所以无法删除换行的原因，其实是，`file_delete_line`在删除前会调用`read_all_lines`，从而在读取代码的时候就给最后一行加了一个换行。
