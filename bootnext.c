#include <efi.h>
#include <efilib.h>

static EFI_GUID global_variable_guid = EFI_GLOBAL_VARIABLE;

static BOOLEAN parse_hex_uint16(
    const CHAR16 *str,
    UINTN char_count,
    UINT16 *value
)
{
    UINTN i = 0;
    UINT32 result = 0;
    BOOLEAN found_digit = FALSE;

    if (str == NULL || value == NULL || char_count == 0) {
        return FALSE;
    }

    // Skip leading spaces
    while (i < char_count &&
           (str[i] == L' ' || str[i] == L'\t')) {
        i++;
    }

    // Optional 0x / 0X prefix
    if ((i + 1) < char_count &&
        str[i] == L'0' &&
        (str[i + 1] == L'x' || str[i + 1] == L'X')) {
        i += 2;
    }

    for (; i < char_count; i++) {
        CHAR16 c = str[i];
        UINT32 digit;

        if (c == L'\0' || c == L' ' || c == L'\t') {
            break;
        }

        if (c >= L'0' && c <= L'9') {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'f') {
            digit = 10 + (c - L'a');
        } else if (c >= L'A' && c <= L'F') {
            digit = 10 + (c - L'A');
        } else {
            return FALSE;
        }

        found_digit = TRUE;
        result = (result << 4) | digit;

        if (result > 0xFFFF) {
            return FALSE;
        }
    }

    if (!found_digit) {
        return FALSE;
    }

    *value = (UINT16)result;
    return TRUE;
}

EFI_STATUS
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE *loaded_image = NULL;
    CHAR16 *load_options;
    UINTN load_options_chars;
    UINT16 boot_next;

    InitializeLib(image_handle, system_table);

    Print(L"bootnext.efi started\r\n");

    status = uefi_call_wrapper(
        BS->HandleProtocol,
        3,
        image_handle,
        &LoadedImageProtocol,
        (VOID **)&loaded_image
    );

    if (EFI_ERROR(status)) {
        Print(L"ERROR: HandleProtocol failed: %r\r\n", status);

        uefi_call_wrapper(
            BS->Stall,
            1,
            5000000
        );

        return status;
    }

    if (loaded_image == NULL ||
        loaded_image->LoadOptions == NULL ||
        loaded_image->LoadOptionsSize < sizeof(CHAR16)) {

        Print(L"ERROR: Missing BootNext argument\r\n");

        uefi_call_wrapper(
            BS->Stall,
            1,
            5000000
        );

        return EFI_INVALID_PARAMETER;
    }

    load_options = (CHAR16 *)loaded_image->LoadOptions;
    load_options_chars =
        loaded_image->LoadOptionsSize / sizeof(CHAR16);

    if (!parse_hex_uint16(
            load_options,
            load_options_chars,
            &boot_next)) {

        Print(L"ERROR: Invalid BootNext argument\r\n");

        uefi_call_wrapper(
            BS->Stall,
            1,
            5000000
        );

        return EFI_INVALID_PARAMETER;
    }

    Print(L"Setting BootNext to %04x\r\n", boot_next);

    status = uefi_call_wrapper(
        RT->SetVariable,
        5,
        L"BootNext",
        &global_variable_guid,
        EFI_VARIABLE_NON_VOLATILE |
        EFI_VARIABLE_BOOTSERVICE_ACCESS |
        EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(boot_next),
        &boot_next
    );

    if (EFI_ERROR(status)) {
        Print(L"ERROR: SetVariable failed: %r\r\n", status);

        uefi_call_wrapper(
            BS->Stall,
            1,
            5000000
        );

        return status;
    }

    Print(L"BootNext set successfully. Rebooting...\r\n");

    uefi_call_wrapper(
        BS->Stall,
        1,
        1000000
    );

    uefi_call_wrapper(
        RT->ResetSystem,
        4,
        EfiResetCold,
        EFI_SUCCESS,
        0,
        NULL
    );

    return EFI_SUCCESS;
}