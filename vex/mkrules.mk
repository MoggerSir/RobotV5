# VEXcode mkrules.mk 2019_03_26_01

# Convierte cada archivo .c en un objeto .o. $< es la fuente; $@ es el archivo que se genera.
$(BUILD)/%.o: %.c $(SRC_H)
	$(Q)$(MKDIR)
	$(ECHO) "CC  $<"
	$(Q)$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	
# Hace lo mismo con archivos .cpp. Si cambian cabeceras o makefile, se vuelve a compilar.
$(BUILD)/%.o: %.cpp $(SRC_H) $(SRC_A)
	$(Q)$(MKDIR)
	$(ECHO) "CXX $<"
	$(Q)$(CXX) $(CXX_FLAGS) $(INC) -c -o $@ $<
	
# Une todos los objetos y las bibliotecas en un ejecutable .elf. $^ son las dependencias.
# Despues muestra cuanto espacio usa el programa.
$(BUILD)/$(PROJECT).elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LINK) $(LNK_FLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

# Extrae del .elf el archivo .bin que se descarga al Brain.
$(BUILD)/$(PROJECT).bin: $(BUILD)/$(PROJECT).elf
	$(Q)$(OBJCOPY) -O binary $(BUILD)/$(PROJECT).elf $(BUILD)/$(PROJECT).bin

# Permite empaquetar objetos en una biblioteca .a. No es el objetivo normal de este proyecto.
$(BUILD)/$(PROJECTLIB).a: $(OBJ)
	$(Q)$(ARCH) $(ARCH_FLAGS) $@ $^

# El objetivo clean borra solo la carpeta de archivos generados de compilacion.
clean:
	$(info clean project)
	$(Q)$(CLEAN)
