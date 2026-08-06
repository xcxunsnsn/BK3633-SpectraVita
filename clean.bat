@echo off

echo clean start...
for /d /r . %%d in (Listings,obj,lst,build) do (
    if exist "%%d" (             
        echo %%d | find /I "\node_modules\" > Nul && ( 
            echo skipping: %%d
        ) || (
            echo deleting: %%d
            rd /s/q "%%d"
        )
    )
)
echo clean end


pause


