#include <efi.h>
#include <efilib.h>

static EFI_GUID global_variable_guid =
    EFI_GLOBAL_VARIABLE;

EFI_STATUS
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    EFI_STATUS status;
    UINT16 boot_next = 0x0008;

    InitializeLib(image_handle, system_table);

    Print(L"Setting UEFI BootNext to Windows Boot Manager (0008)...\r\n");

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

    Print(L"BootNext set. Rebooting...\r\n");
    BS->Stall(1000000);

    RT->ResetSystem(
        EfiResetCold,
        EFI_SUCCESS,
        0,
        NULL
    );

    return EFI_SUCCESS;
}

