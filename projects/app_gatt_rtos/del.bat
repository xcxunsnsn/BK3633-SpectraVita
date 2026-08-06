if exist ..\..\..\..\delete_me.config (
echo delete exit ) else ( 
del ..\..\..\readme.docx  /s/q
rd ..\..\projects\stack /s /q
rd ..\..\projects\controller_lib /s /q
rd ..\..\projects\all_code_compile /s /q
rd ..\..\projects\stack_all_roles /s /q
rd ..\..\projects\stack_all_roles_lib_jtag /s /q
del ..\..\src\ip\*.c /s /q
del ..\..\src\modules\*.c /s /q
del ..\..\..\..\bk3633_V*.rar  /s/q
)
