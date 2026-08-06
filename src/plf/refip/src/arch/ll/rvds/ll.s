;/**
; ****************************************************************************************
; *
; * @file ll.s
; *
; * @brief ARM low level functions.
; *
; * Copyright (C) RivieraWaves 2009-2015
; *
; * $Rev:  $
; *
; ****************************************************************************************
; */


    AREA ||.text||, CODE, READONLY

    IF {CPU} /= "Cortex-M3"
    IF {CPU} /= "Cortex-M1"
    IF {CPU} /= "Cortex-M0"
    CODE32
    ENDIF
    ENDIF
    ENDIF


   END

