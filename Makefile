CFLAGS = -std=c17 -Wall -Wextra -Werror -pedantic -g

LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

BUILD_DIR = build/

VulkanProbe: main.c
	mkdir -p $(BUILD_DIR)
	clang $(CFLAGS) -o $(BUILD_DIR)VulkanProbe main.c $(LDFLAGS)

.PHONY: test clean mac

run: VulkanProbe
	./$(BUILD_DIR)VulkanProbe

mac: VulkanProbe
	install_name_tool -add_rpath /usr/local/lib/ ./build/VulkanProbe
	./$(BUILD_DIR)VulkanProbe

clean:
	rm -rf $(BUILD_DIR)