automake --add-missing
./configure --target=x86_64 --with-platform=efi
make -j10
grub-mkimage -d grub-core -c x86_64-efi.cfg -p /EFI/grub -o bootx64.efi -O x86_64-efi part_gpt part_msdos disk fat exfat ext2 ntfs xfs appleldr hfs iso9660 normal search_fs_file configfile linux linux16 chain loopback echo efi_gop efi_uga video_bochs video_cirrus file gfxmenu gfxterm gfxterm_background gfxterm_menu halt reboot help jpeg ls png true videoinfo

cp bootx64.efi out/grub/