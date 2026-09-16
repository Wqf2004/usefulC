# 注意事项

当前dfm/src/file_op.c中的file_delete_line无法删除换行，也就是无法将已存在的行取消，而只能删除这一行里的内容，故待操作的文件data/b.txt，初始化时除了说明行，还需要加一个换行。
