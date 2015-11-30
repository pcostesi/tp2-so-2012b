#ifndef MODULELOADER_H
#define MODULELOADER_H
#define MODULE_NAME_SIZE 128

struct module_entry {
	void * start;
	uint64_t size;
	char name[MODULE_NAME_SIZE];
};

uint64_t ldr_module_section_size(void * module_section_start);
void * ldr_module_load(void * module_section_start, char * module_name, struct module_entry * entry);
uint8_t loadModules(void * payloadStart, void ** moduleTargetAddress);

#endif