#include "file.h"

static efi_status open_root(efi_handle image_handle,
	efi_system_table_t *system_table,
	efi_file_protocol_t **root)
{
	efi_guid_t loaded_image_guid = efi_loaded_image_protocol_guid();
	efi_guid_t simple_fs_guid = efi_simple_file_system_protocol_guid();
	efi_loaded_image_protocol_t *loaded_image;
	efi_simple_file_system_protocol_t *fs;
	efi_status status;

	status = system_table->boot_services->handle_protocol(
		image_handle, &loaded_image_guid, (void **)&loaded_image);
	if (status != EFI_SUCCESS) {
		return status;
	}

	status = system_table->boot_services->handle_protocol(
		loaded_image->device_handle, &simple_fs_guid, (void **)&fs);
	if (status != EFI_SUCCESS) {
		return status;
	}

	return fs->open_volume(fs, root);
}

efi_status boot_read_file(efi_handle image_handle,
	efi_system_table_t *system_table,
	efi_char16_t *path,
	void **buffer,
	uint64_t *size)
{
	efi_guid_t file_info_guid = efi_file_info_guid();
	efi_file_protocol_t *root;
	efi_file_protocol_t *file;
	efi_file_info_t *file_info;
	efi_uintn_t info_size;
	efi_uintn_t read_size;
	efi_status status;

	status = open_root(image_handle, system_table, &root);
	if (status != EFI_SUCCESS) {
		return status;
	}

	status = root->open(root, &file, path, EFI_OPEN_MODE_READ, 0);
	root->close(root);
	if (status != EFI_SUCCESS) {
		return status;
	}

	info_size = 0;
	status = file->get_info(file, &file_info_guid, &info_size, 0);
	if (status != EFI_BUFFER_TOO_SMALL) {
		file->close(file);
		return status;
	}

	status = system_table->boot_services->allocate_pool(
		EFI_LOADER_DATA, info_size, (void **)&file_info);
	if (status != EFI_SUCCESS) {
		file->close(file);
		return status;
	}

	status = file->get_info(file, &file_info_guid, &info_size, file_info);
	if (status != EFI_SUCCESS) {
		file->close(file);
		return status;
	}

	*size = file_info->file_size;
	status = system_table->boot_services->allocate_pool(
		EFI_LOADER_DATA, *size, buffer);
	if (status != EFI_SUCCESS) {
		file->close(file);
		return status;
	}

	read_size = *size;
	status = file->read(file, &read_size, *buffer);
	file->close(file);
	if (status != EFI_SUCCESS) {
		return status;
	}

	if (read_size != *size) {
		return EFI_LOAD_ERROR;
	}

	return EFI_SUCCESS;
}
