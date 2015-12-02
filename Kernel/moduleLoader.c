#include <stdint.h>
#include <lib.h>
#include <string.h>
#include <stdio.h>
#include <moduleLoader.h>

static void loadModule(uint8_t ** module, void * targetModuleAddress);
static uint32_t readUint32(uint8_t ** address);

static void print_brief(uint8_t * start, int max)
{
	int i;
	for (i = 0; i < max; i++) {
		printf("%x%x ", start[2 * i], start[2 * i + 1]);
	}
	puts("\n");
}

uint64_t ldr_module_section_size(void * module_section_start)
{
	int i;
	uint32_t moduleSectionSize = sizeof(uint8_t);
	uint8_t * currentModule = (uint8_t*)module_section_start;
	uint32_t moduleCount = readUint32(&currentModule);
	uint32_t moduleSize;

	for (i = 0; i < moduleCount; i++) {
		moduleSize = readUint32(&currentModule);
		currentModule += (moduleSize + MODULE_NAME_SIZE);
		moduleSectionSize += moduleSize + MODULE_NAME_SIZE + sizeof(uint32_t);
	}
	return moduleSectionSize;
}

void * ldr_module_load(void * module_section_start, char * module_name, struct module_entry * entry)
{
	int i;
	char buffer[MODULE_NAME_SIZE] = {0};
	uint8_t * currentModule = (uint8_t*)module_section_start;
	uint32_t moduleCount = readUint32(&currentModule);
	uint32_t moduleSize;

	for (i = 0; i < moduleCount; i++) {
		moduleSize = readUint32(&currentModule);
		memset(buffer, 0, MODULE_NAME_SIZE);
		memcpy(buffer, currentModule, MODULE_NAME_SIZE);
		currentModule += sizeof(buffer);
		if (strcmp(buffer, module_name) == 0) {
			entry->start = currentModule;
			entry->size = moduleSize;
			print_brief(entry->start, 16);
			memcpy(entry->name, buffer, MODULE_NAME_SIZE);
			return currentModule;
		}
		currentModule += moduleSize;
	}
	return NULL;
}


uint8_t loadModules(void * payloadStart, void ** targetModuleAddress)
{
	int i;
	uint8_t * currentModule = (uint8_t*)payloadStart;
	uint32_t moduleCount = readUint32(&currentModule);

	for (i = 0; i < moduleCount; i++)
		loadModule(&currentModule, targetModuleAddress[i]);
	return moduleCount;
}

static void loadModule(uint8_t ** module, void * targetModuleAddress)
{
	uint32_t moduleSize = readUint32(module);

	memcpy(targetModuleAddress, *module, moduleSize);
	*module += moduleSize;
}

static uint32_t readUint32(uint8_t ** address)
{
	uint32_t result = *(uint32_t*)(*address);
	*address += sizeof(uint32_t);
	return result;
}
