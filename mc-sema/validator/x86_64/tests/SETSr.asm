BITS 64
;TEST_FILE_META_BEGIN
;TEST_TYPE=TEST_F
;TEST_IGNOREFLAGS=
;TEST_FILE_META_END
    mov ecx, 0x0
    mov ebx, 0x222
    cmp ecx, ebx
    ;TEST_BEGIN_RECORDING
    sets al
    ;TEST_END_RECORDING 