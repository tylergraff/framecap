PROJECTS = common apps

.PHONY: target clean test coverage $(PROJECTS)

CC = gcc


common:
        cd common && $(MAKE)

apps:
        cd apps && $(MAKE)


clean:
        cd common && $(MAKE) clean
        cd apps && $(MAKE) clean
