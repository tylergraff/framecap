PROJECTS = common apps

.PHONY: target clean $(PROJECTS)

CC = gcc

target: $(PROJECTS)
common:
	cd common && $(MAKE)

apps:
	cd apps && $(MAKE)


clean:
	cd common && $(MAKE) clean
	cd apps && $(MAKE) clean
