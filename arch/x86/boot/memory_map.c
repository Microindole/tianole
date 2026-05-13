#include "memory_map.h"

/**
 * fetch_memory_map() - Fetch a boot-services memory map and map key.
 * @system_table: UEFI system table providing memory services.
 * @boot_info: Handoff structure updated with map address, size and key.
 *
 * UEFI requires the caller to discover the needed buffer size first. The extra
 * descriptor slack absorbs small firmware allocations between the size probe
 * and the second GetMemoryMap() call.
 *
 * Return: EFI_SUCCESS on success or an EFI error status.
 */
static efi_status fetch_memory_map(
	efi_system_table_t *system_table, boot_info_t *boot_info)
{
	boot_memory_descriptor_t *memory_map;
	efi_uintn_t memory_map_size;
	efi_uintn_t map_key;
	efi_uintn_t descriptor_size;
	uint32_t descriptor_version;
	efi_status status;

	memory_map_size = 0;
	map_key = 0;
	descriptor_size = 0;
	descriptor_version = 0;
	status = system_table->boot_services->get_memory_map(&memory_map_size,
		0,
		&map_key,
		&descriptor_size,
		&descriptor_version);
	if (status != EFI_BUFFER_TOO_SMALL) {
		return status;
	}

	memory_map_size += descriptor_size * 8;
	status = system_table->boot_services->allocate_pool(
		EFI_LOADER_DATA, memory_map_size, (void **)&memory_map);
	if (status != EFI_SUCCESS) {
		return status;
	}

	status = system_table->boot_services->get_memory_map(&memory_map_size,
		memory_map,
		&map_key,
		&descriptor_size,
		&descriptor_version);
	if (status != EFI_SUCCESS) {
		system_table->boot_services->free_pool(memory_map);
		return status;
	}

	boot_info->memory_map = (uint64_t)(uintptr_t)memory_map;
	boot_info->memory_map_size = memory_map_size;
	boot_info->memory_map_key = map_key;
	boot_info->memory_descriptor_size = descriptor_size;
	boot_info->memory_descriptor_version = descriptor_version;

	return EFI_SUCCESS;
}

/**
 * boot_exit_services_with_latest_memory_map() - Finalize firmware handoff.
 * @image_handle: UEFI image handle passed to ExitBootServices().
 * @system_table: UEFI system table.
 * @boot_info: Boot handoff structure updated with the final memory map.
 *
 * ExitBootServices() can fail if the map key became stale. Retrying with a
 * freshly fetched map keeps the boot path robust without hiding other errors.
 *
 * Return: EFI_SUCCESS on success or an EFI error status.
 */
efi_status boot_exit_services_with_latest_memory_map(efi_handle image_handle,
	efi_system_table_t *system_table,
	boot_info_t *boot_info)
{
	efi_status status;
	uint32_t exit_try;

	for (exit_try = 0; exit_try < 2; ++exit_try) {
		status = fetch_memory_map(system_table, boot_info);
		if (status != EFI_SUCCESS) {
			return status;
		}

		status = system_table->boot_services->exit_boot_services(
			image_handle, boot_info->memory_map_key);
		if (status == EFI_SUCCESS) {
			boot_info->boot_flags &= ~BOOT_FLAG_SERVICES_ACTIVE;
			return EFI_SUCCESS;
		}
	}

	return status;
}
