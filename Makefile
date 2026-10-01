APP      := bootnext

CC       := gcc
LD       := ld
OBJCOPY  := objcopy
SBSIGN   := sbsign
SBVERIFY := sbverify

EFI_INC      := /usr/include/efi
EFI_ARCH_INC := /usr/include/efi/x86_64

EFI_CRT   := /usr/lib/crt0-efi-x86_64.o
EFI_LDS   := /usr/lib/elf_x86_64_efi.lds
EFI_LIBDIR := /usr/lib

MOK_KEY  := /var/lib/dkms/mok.key
MOK_CERT := /var/lib/dkms/mok.pub

ESP_DIR  := /boot/efi/EFI/debian

CFLAGS := \
	-I$(EFI_INC) \
	-I$(EFI_ARCH_INC) \
	-fpic \
	-fshort-wchar \
	-mno-red-zone \
	-fno-stack-protector \
	-DEFI_FUNCTION_WRAPPER

LDFLAGS := \
	-nostdlib \
	-znocombreloc \
	-T $(EFI_LDS) \
	-shared \
	-Bsymbolic \
	$(EFI_CRT) \
	-L$(EFI_LIBDIR) \
	-lefi \
	-lgnuefi

EFI_SECTIONS := \
	-j .text \
	-j .sdata \
	-j .data \
	-j .dynamic \
	-j .dynsym \
	-j .rel \
	-j .rela \
	-j .reloc

.PHONY: all sign verify install clean check

all: $(APP)-signed.efi

check:
	@test -f $(EFI_CRT) || (echo "Missing $(EFI_CRT)" && exit 1)
	@test -f $(EFI_LDS) || (echo "Missing $(EFI_LDS)" && exit 1)
	@test -f $(MOK_KEY) || (echo "Missing $(MOK_KEY)" && exit 1)
	@test -f $(MOK_CERT) || (echo "Missing $(MOK_CERT)" && exit 1)

$(APP).o: $(APP).c
	$(CC) $(CFLAGS) -c $< -o $@

$(APP).so: $(APP).o
	$(LD) $(LDFLAGS) $(APP).o -o $@

$(APP).efi: $(APP).so
	$(OBJCOPY) $(EFI_SECTIONS) \
		--target=efi-app-x86_64 \
		$< $@

$(APP)-signed.efi: check $(APP).efi
	sudo $(SBSIGN) \
		--key $(MOK_KEY) \
		--cert $(MOK_CERT) \
		--output $@ \
		$(APP).efi

sign: $(APP)-signed.efi

verify: $(APP)-signed.efi
	$(SBVERIFY) --list $(APP)-signed.efi
	file $(APP)-signed.efi

install: $(APP)-signed.efi
	sudo cp $(APP)-signed.efi $(ESP_DIR)/$(APP).efi
	sudo sync
	@echo "Installed to $(ESP_DIR)/$(APP).efi"

clean:
	rm -f \
		$(APP).o \
		$(APP).so \
		$(APP).efi \
		$(APP)-signed.efi