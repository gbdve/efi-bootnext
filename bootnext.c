#include <efi.h>
#include <efilib.h>

static EFI_GUID global_variable_guid = EFI_GLOBAL_VARIABLE;

static BOOLEAN parse_hex_uint16(CHAR16 *str, UINT16 *value)
{
    UINTN i;
    UINT32 result = 0;
    BOOLEAN found_digit = FALSE;

    if (str == NULL || value == NULL) {
        return FALSE;
    }

    // Skip leading spaces
    while (*str == L' ' || *str == L'\t') {
        str++;
    }

    // Optional 0x / 0X prefix
    if (str[0] == L'0' && (str[1] == L'x' || str[1] == L'X')) {
        str += 2;
    }

    for (i = 0; str[i] != L'\0'; i++) {
        CHAR16 c = str[i];
        UINT32 digit;

        if (c == L' ' || c == L'\t') {
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
    EFI_LOADED_IMAGE *loaded_image;
    CHAR16 *load_options;
    UINT16 boot_next;

    InitializeLib(image_handle, system_table);

    status = BS->HandleProtocol(
        image_handle,
        &LoadedImageProtocol,
        (VOID **)&loaded_image
    );

    if (EFI_ERROR(status)) {
        Print(L"ERROR: Cannot access LoadedImage protocol: %r\r\n", status);
        BS->Stall(5000000);
        return status;
    }

    load_options = (CHAR16 *)loaded_image->LoadOptions;

    if (loaded_image->LoadOptionsSize == 0 || load_options == NULL) {
        Print(L"ERROR: Missing BootNext argument.\r\n");
        Print(L"Usage example: bootnext.efi 0008\r\n");
        BS->Stall(5000000);
        return EFI_INVALID_PARAMETER;
    }

    if (!parse_hex_uint16(load_options, &boot_next)) {
        Print(L"ERROR: Invalid BootNext value: '%s'\r\n", load_options);
        BS->Stall(5000000);
        return EFI_INVALID_PARAMETER;
    }

    Print(L"Setting UEFI BootNext to %04x...\r\n", boot_next);

    status = RT->SetVariable(
        L"BootNext",
        &global_variable_guid,
        EFI_VARIABLE_NON_VOLATILE |
        EFI_VARIABLE_BOOTSERVICE_ACCESS |
        EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(boot_next),
        &boot_next
    );

    if (EFI_ERROR(status)) {
        Print(L"ERROR: SetVariable(BootNext) failed: %r\r\n", status);
        BS->Stall(5000000);
        return status;
    }

    Print(L"BootNext set to %04x. Rebooting...\r\n", boot_next);
    BS->Stall(1000000);

    RT->ResetSystem(
        EfiResetCold,
        EFI_SUCCESS,
        0,
        NULL
    );

    return EFI_SUCCESS;
}