del bk3633.* 
del *.bak
del controller_hci*.docx
del ..\*.docx  /s/q
del ..\.gitignore /s/q
rd ..\.git /s/q

rd projects\controller_lib /s /q
rd projects\stack /s /q
rd projects\stack_multi_connect /s /q
rd projects\stack_all_roles /s /q
rd projects\stack_all_roles_lib_jtag /s /q
rd projects\stack_central /s /q
rd projects\stack_mouse /s /q
rd projects\stack_codesize_cut /s /q
rd projects\all_code_compile /s /q


del src\ip\*.c /s /q
del src\modules\*.c /s /q
del src\*.xls /s /q
del src\*.xlsx /s /q

rd projects\app_2.4g\app_usb_2.4g\obj /s /q
rd projects\app_2.4g\boot_usb\obj /s /q

rd projects\app_ancs\obj /s /q
rd projects\app_controller\obj /s /q
rd projects\app_gatt\obj /s /q
rd projects\app_gatt_multi_connect\obj /s /q
rd projects\app_gatt_all_roles\obj /s /q
rd projects\app_gatt_all_roles_jtag\obj /s /q
rd projects\app_gatt_all_roles_for_mesh\obj /s /q
rd projects\app_gatt_central\obj /s /q
rd projects\app_gatt_ftmp\obj /s /q
rd projects\app_mouse\obj /s /q
rd projects\app_rc\obj /s /q
rd projects\app_multi_bond\obj /s /q

rd projects\boot\obj /s /q
rd projects\boot_for_all_roles\obj /s /q
rd projects\boot_usb_central\obj /s /q
rd projects\boot_usb_for_mouse\obj /s /q

del ..\..\bk3633_V*.rar  /s/q

