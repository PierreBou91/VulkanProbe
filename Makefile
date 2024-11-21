CFLAGS = -std=c17 -Wall -Wextra -Werror -pedantic -g

LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

BUILD_DIR = build/

VulkanProbe: main.c
	clang $(CFLAGS) -o $(BUILD_DIR)VulkanProbe main.c $(LDFLAGS)

.PHONY: test clean

test: VulkanProbe
	./$(BUILD_DIR)VulkanProbe

clean:
	rm -rf $(BUILD_DIR)