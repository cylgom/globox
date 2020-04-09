#define _XOPEN_SOURCE 700
#include "globox.h"

#ifdef GLOBOX_X11
	#define VK_USE_PLATFORM_XCB_KHR 1
	#define PLATFORM_DEPENDENT_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

struct globox ctx = {0};

// vulkan devices info
struct vlk_dev_info_list
{
	VkBool32* support_list;
	VkQueueFamilyProperties* family_props_list;
	VkSurfaceFormatKHR* surf_format_list;
};

VkDevice vlk_dev_logical;
VkQueue vlk_queue;

VkSwapchainKHR vlk_chain;
VkCommandPool vlk_cmd_pool;
VkImage* vlk_image_list;
VkCommandBuffer* vlk_cmd_list;

VkSemaphore vlk_present_semaphore;
VkSemaphore vlk_render_semaphore;

VkPhysicalDevice* vlk_dev_list;
VkPhysicalDeviceProperties* vlk_dev_props_list;
VkSurfaceCapabilitiesKHR* vlk_surf_caps_list;
struct vlk_dev_info_list* vlk_dev_info_list;

uint32_t count_dev;
uint32_t count_queue;
uint32_t count_format;
uint32_t count_chain;
struct vlk_dev* vlk_dev;

uint32_t vlk_selected_dev;
uint32_t vlk_selected_family;

VkDebugReportCallbackEXT vlk_debug;

// optional vulkan app info
VkApplicationInfo vlk_app_info = {0};

// info for vulkan app info
const char* vlk_app_name = "Globox Screen Clearing";
const char* vlk_engine_name = "Globox Vulkan Example";

// optional vulkan extensions info
const char* ext[] =
{
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef GLOBOX_X11
	VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
};

// optional vulkan layers info
const char* layers[] =
{
	"VK_LAYER_LUNARG_standard_validation"
};

// print available vulkan extensions
uint8_t vlk_print_ext()
{
	VkExtensionProperties* vlk_ext;
	uint32_t vlk_ext_count;
	VkResult vlk_ok;

	vlk_ok = vkEnumerateInstanceExtensionProperties(
		NULL,
		&vlk_ext_count,
		NULL);

	if (vlk_ok != VK_SUCCESS)
	{
		return 1;
	}

	vlk_ext = malloc(vlk_ext_count * (sizeof (VkExtensionProperties)));
	vlk_ok = vkEnumerateInstanceExtensionProperties(
		NULL,
		&vlk_ext_count,
		vlk_ext);

	if (vlk_ok != VK_SUCCESS)
	{
		free(vlk_ext);

		return 2;
	}

	printf("# Vulkan extensions available\n");

	for (uint32_t i = 0; i < vlk_ext_count; ++i)
	{
		printf("%s\n", vlk_ext[i].extensionName);
	}

	printf("\n");

	free(vlk_ext);

	return 0;
}

// fill optional vulkan structures
void vlk_prepare(struct globox* globox)
{
	vlk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vlk_app_info.pNext = NULL;
	vlk_app_info.pApplicationName = vlk_app_name;
	vlk_app_info.applicationVersion = 1;
	vlk_app_info.pEngineName = vlk_engine_name;
	vlk_app_info.engineVersion = 1;
	vlk_app_info.apiVersion = VK_API_VERSION_1_0;

	globox->vlk_app_info = &vlk_app_info;
	globox->vlk_layers_count = (sizeof (layers)) / (sizeof (char*));
	globox->vlk_layers_names = layers;
	globox->vlk_ext_count = (sizeof (ext)) / (sizeof (char*));
	globox->vlk_ext_names = ext;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vlk_debug_callback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT type,
	uint64_t object,
	size_t location,
	int32_t message_code,
	const char* layer_prefix,
	const char* message,
	void* user_data)
{
	printf("# Vulkan error\n%s\n\n", message);
	return VK_FALSE;
}

// validation layers
uint8_t vlk_validation_init(struct globox* globox)
{
	PFN_vkCreateDebugReportCallbackEXT callback_creator =
		(PFN_vkCreateDebugReportCallbackEXT)
		vkGetInstanceProcAddr(
			globox->vlk_instance,
			"vkCreateDebugReportCallbackEXT");

	VkDebugReportCallbackCreateInfoEXT info =
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
			| VK_DEBUG_REPORT_WARNING_BIT_EXT
			| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
		.pfnCallback = vlk_debug_callback,
		.pUserData = NULL,
	};

	VkResult ok = callback_creator(
		globox->vlk_instance,
		&info,
		NULL,
		&vlk_debug);

	if (ok != VK_SUCCESS)
	{
		return 1;
	}

	return 0;
}

uint8_t vlk_list_physical(struct globox* globox)
{
	count_dev = 0;

	VkResult ok = vkEnumeratePhysicalDevices(
		globox->vlk_instance,
		&count_dev,
		NULL);

	if (ok != VK_SUCCESS)
	{
		return 1;
	}

	vlk_dev_list = malloc(
		count_dev * (sizeof (VkPhysicalDevice)));
	vlk_dev_props_list = malloc(
		count_dev * (sizeof (VkPhysicalDeviceProperties)));
	vlk_surf_caps_list = malloc(
		count_dev * (sizeof (VkSurfaceCapabilitiesKHR)));
	vlk_dev_info_list = malloc(
		count_dev * (sizeof (VkPhysicalDevice)));

	ok = vkEnumeratePhysicalDevices(
		globox->vlk_instance,
		&count_dev,
		vlk_dev_list);

	if (ok != VK_SUCCESS)
	{
		return 2;
	}

	printf("# Vulkan-compatible hardware\n");

	for (uint32_t i = 0; i < count_dev; ++i)
	{
		const VkPhysicalDevice* dev = &(vlk_dev_list[i]);

		vkGetPhysicalDeviceProperties(*dev, &(vlk_dev_props_list[i]));

		count_queue = 0;

		vkGetPhysicalDeviceQueueFamilyProperties(*dev, &count_queue, NULL);

		vlk_dev_info_list[i].support_list =
			malloc(count_queue * (sizeof (VkBool32)));

		vlk_dev_info_list[i].family_props_list =
			malloc(count_queue * (sizeof (VkQueueFamilyProperties)));

		vkGetPhysicalDeviceQueueFamilyProperties(
			*dev,
			&count_queue,
			vlk_dev_info_list[i].family_props_list);

		for (uint32_t k = 0; k < count_queue; ++k)
		{
			ok = vkGetPhysicalDeviceSurfaceSupportKHR(
				*dev,
				k,
				ctx.vlk_surface,
				vlk_dev_info_list[i].support_list);

			if (ok != VK_SUCCESS)
			{
				return 3;
			}
		}

		count_format = 0;

		vkGetPhysicalDeviceSurfaceFormatsKHR(
			*dev,
			ctx.vlk_surface,
			&count_format,
			NULL);

		if (count_format == 0)
		{
			return 4;
		}

		vlk_dev_info_list[i].surf_format_list =
			malloc(count_format * (sizeof (VkSurfaceFormatKHR)));

		ok = vkGetPhysicalDeviceSurfaceFormatsKHR(
			*dev,
			ctx.vlk_surface,
			&count_format,
			vlk_dev_info_list[i].surf_format_list);

		if (ok != VK_SUCCESS)
		{
			return 5;
		}

		uint32_t api = vlk_dev_props_list[i].apiVersion;

		printf(
			"%s - Vulkan %d.%d.%d - %d queues\n",
			vlk_dev_props_list[i].deviceName,
			VK_VERSION_MAJOR(api),
			VK_VERSION_MINOR(api),
			VK_VERSION_PATCH(api),
			count_queue);

		for (uint32_t k = 0; k < count_format; ++k)
		{
			const VkSurfaceFormatKHR surf_format =
				vlk_dev_info_list[i].surf_format_list[k];

			printf(
				" format %d color space %d\n",
				surf_format.format,
				surf_format.colorSpace);
		}

		ok = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			*dev,
			ctx.vlk_surface,
			vlk_surf_caps_list);

		if (ok != VK_SUCCESS)
		{
			return 6;
		}

		switch (vlk_surf_caps_list[i].supportedUsageFlags)
		{
			case VK_IMAGE_USAGE_TRANSFER_SRC_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_TRANSFER_SRC_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_TRANSFER_DST_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_TRANSFER_DST_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_SAMPLED_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_SAMPLED_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_STORAGE_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_STORAGE_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT:
			{
				printf(" flag \"VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT\"\n");
				break;
			}
			case VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV:
			{
				printf(" flag \"VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV\"\n");
				break;
			}
			case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT:
			{
				printf(" flag \"VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT\"\n");
				break;
			}
			default:
			{
				printf(" flag unkown\n");
				break;
			}
		}
	}

	printf("\n");

	return 0;
}

uint8_t vlk_select_physical(struct globox* globox)
{
	uint32_t i = 0;
	uint32_t k = 0;

	printf("# Vulkan hardware selection\n");

	while (i < count_dev)
	{
		while (k < count_queue)
		{
			VkQueueFamilyProperties* family_prop =
				&(vlk_dev_info_list[i].family_props_list[k]);
			VkQueueFlags flags = family_prop->queueFlags;

			printf(
				"family %d: %d queues\n",
				k,
				family_prop->queueCount);

			printf(
				" GFX %d Compute %d Transfer %d Sparse binding %d\n",
				flags & VK_QUEUE_GRAPHICS_BIT ? 1 : 0,
				flags & VK_QUEUE_COMPUTE_BIT ? 1 : 0,
				flags & VK_QUEUE_TRANSFER_BIT ? 1 : 0,
				flags & VK_QUEUE_SPARSE_BINDING_BIT ? 1 : 0);

			if (flags & VK_QUEUE_GRAPHICS_BIT)
			{
				if (vlk_dev_info_list[i].support_list[k])
				{
					printf(" Present is not supported\n");
					++k;

					continue;
				}

				printf("\n# Using device %d and queue family %d\n\n", i, k);

				vlk_selected_dev = i;
				vlk_selected_family = k;

				return 0;
			}

			++k;
		}

		++i;
	}

	printf("# No suitable device found\n");

	return 1;
}

void vlk_free_physical()
{
	free(vlk_dev_list);
	free(vlk_surf_caps_list);
	free(vlk_dev_info_list);
}

uint8_t vlk_create_logical(struct globox* globox)
{
	float priorities = 1.0f;

	VkDeviceQueueCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = vlk_selected_family,
		.queueCount = 1,
		.pQueuePriorities = &priorities,
	};

	const char* ext[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkDeviceCreateInfo dev =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.enabledExtensionCount = (sizeof (ext)) / (sizeof (char*)),
		.ppEnabledExtensionNames = ext,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &info,
	};

	VkResult ok = vkCreateDevice(
		vlk_dev_list[vlk_selected_dev],
		&dev,
		NULL,
		&vlk_dev_logical);

	if (ok != VK_SUCCESS)
	{
		return 1;
	}

	printf("device created\n");

	return 0;
}

void handler()
{
	globox_handle_events(&ctx);

	if (ctx.redraw)
	{
		uint32_t index = 0;

		VkResult ok = vkAcquireNextImageKHR(
			vlk_dev_logical,
			vlk_chain,
			0xFFFFFFFFFFFFFFFF,
			vlk_present_semaphore,
			NULL,
			&index);

		if (ok != VK_SUCCESS)
		{
			printf("SHEEEIIIT %d\n", ok);
			return;
		}

		VkPipelineStageFlags flags = VK_PIPELINE_STAGE_TRANSFER_BIT;

		VkSubmitInfo submit =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &(vlk_cmd_list[index]),
			.pWaitSemaphores = &vlk_present_semaphore,
			.waitSemaphoreCount = 1,
			.pWaitDstStageMask = &flags,
			.pSignalSemaphores = &vlk_render_semaphore,
			.signalSemaphoreCount = 1,
		};

		printf("1\n");
		ok = vkQueueSubmit(vlk_queue, 1, &submit, NULL);
		printf("2\n");

		if (ok != VK_SUCCESS)
		{
			printf("2 SHEEEIIIT\n");
			return;
		}

		VkPresentInfoKHR present =
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.swapchainCount = 1,
			.pSwapchains = &vlk_chain,
			.pImageIndices = &index,
			.pWaitSemaphores = &vlk_render_semaphore,
			.waitSemaphoreCount = 1,
		};

		ok = vkQueuePresentKHR(vlk_queue, &present);

		if (ok != VK_SUCCESS)
		{
			printf("3 SHEEEIIIT\n");
			return;
		}

		globox_copy(&ctx, 0, 0, ctx.width, ctx.height);
	}
}

uint8_t vlk_swapchain(struct globox* globox)
{
	uint32_t count_image = 2;
	VkSurfaceCapabilitiesKHR surf_caps = vlk_surf_caps_list[vlk_selected_dev];

	if ((surf_caps.currentExtent.width == ((uint32_t) -1))
	|| (count_image < surf_caps.minImageCount)
	|| (count_image > surf_caps.maxImageCount))
	{
		return 1;
	}

	// TODO: check surface format list
	VkSwapchainCreateInfoKHR info =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = globox->vlk_surface,
		.minImageCount = count_image,
		.imageFormat =
			vlk_dev_info_list[vlk_selected_dev]
				.surf_format_list[0]
				.format,
		.imageColorSpace =
			vlk_dev_info_list[vlk_selected_dev]
				.surf_format_list[0]
				.colorSpace,
		.imageExtent = vlk_surf_caps_list[vlk_selected_dev].currentExtent,
		.imageUsage =
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = true,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	};

	VkResult ok = vkCreateSwapchainKHR(
		vlk_dev_logical,
		&info,
		NULL,
		&vlk_chain);

	if (ok != VK_SUCCESS)
	{
		return 2;
	}

	count_chain = 0;

	ok = vkGetSwapchainImagesKHR(
		vlk_dev_logical,
		vlk_chain,
		&count_chain,
		NULL);

	if (ok != VK_SUCCESS)
	{
		return 3;
	}

	vlk_image_list = malloc(count_chain * (sizeof (VkImage)));
	vlk_cmd_list = malloc(count_chain * (sizeof (VkCommandBuffer)));

	ok = vkGetSwapchainImagesKHR(
		vlk_dev_logical,
		vlk_chain,
		&count_chain,
		vlk_image_list);

	if (ok != VK_SUCCESS)
	{
		return 4;
	}

	return 0;
}

uint8_t vlk_cmd_get()
{
	VkCommandPoolCreateInfo cmd =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = vlk_selected_family,
	};

	VkResult ok = vkCreateCommandPool(
		vlk_dev_logical,
		&cmd,
		NULL,
		&vlk_cmd_pool);

	if (ok != VK_SUCCESS)
	{
		return 1;
	}

	VkCommandBufferAllocateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vlk_cmd_pool,
		.commandBufferCount = count_chain,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	};

	ok = vkAllocateCommandBuffers(
		vlk_dev_logical,
		&info,
		vlk_cmd_list);

	if (ok != VK_SUCCESS)
	{
		return 2;
	}

	return 0;
}

uint8_t vlk_cmd_rec()
{
	VkCommandBufferBeginInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
	};

	VkClearColorValue color;
	color.float32[0] = 164.0f / 256.0f;
	color.float32[1] = 30.0f / 256.0f;
	color.float32[2] = 34.0f / 256.0f;
	color.float32[3] = 0.0f;

	VkImageSubresourceRange range =
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.levelCount = 1,
		.layerCount = 1,
	};

	for (uint32_t i = 0; i < count_chain; ++i)
	{
		VkResult ok = vkBeginCommandBuffer(vlk_cmd_list[i], &info);

		if (ok != VK_SUCCESS)
		{
			return 1;
		}

		vkCmdClearColorImage(
			vlk_cmd_list[i],
			vlk_image_list[i],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			&color,
			1,
			&range);

		ok = vkEndCommandBuffer(vlk_cmd_list[i]);

		if (ok != VK_SUCCESS)
		{
			return 2;
		}
	}

	return 0;
}

uint32_t vlk_semaphores()
{
	VkResult ok;

	VkSemaphoreCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	ok = vkCreateSemaphore(vlk_dev_logical , &info, NULL, &vlk_present_semaphore);

	if (ok != VK_SUCCESS)
	{
		return 1;
	}

	ok = vkCreateSemaphore(vlk_dev_logical , &info, NULL, &vlk_render_semaphore);

	if (ok != VK_SUCCESS)
	{
		return 2;
	}

	return 0;
}

int main()
{
	// show available vulkan extensions
	vlk_print_ext();
	// add optional vulkan info in globox
	vlk_prepare(&ctx);

	VkInstanceCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = ctx.vlk_chain,
		.flags = 0,
		.pApplicationInfo = ctx.vlk_app_info,
		.enabledLayerCount = ctx.vlk_layers_count,
		.ppEnabledLayerNames = ctx.vlk_layers_names,
		.enabledExtensionCount = ctx.vlk_ext_count,
		.ppEnabledExtensionNames = ctx.vlk_ext_names,
	};

	VkResult vlk_ok = vkCreateInstance(&info, NULL, &ctx.vlk_instance);
	
	if (vlk_ok != VK_SUCCESS)
	{
		return 1;
	}

	// basic vulkan config
	vlk_validation_init(&ctx);

	// create window
	bool ok = globox_open(
		&ctx,
		GLOBOX_STATE_REGULAR,
		"test",
		0,
		0,
		100,
		100,
		true);

	if (!ok)
	{
		return 1;
	}

	vlk_list_physical(&ctx);
	vlk_select_physical(&ctx);
	vlk_create_logical(&ctx);

	vkGetDeviceQueue(vlk_dev_logical, vlk_selected_family, 0, &vlk_queue);

	uint8_t globox_ok = vlk_swapchain(&ctx);

	if (globox_ok != 0)
	{
		printf("shit1 %d\n", globox_ok);
		return 1;
	}

	globox_ok = vlk_cmd_get();

	if (globox_ok != 0)
	{
		printf("shit2\n");
		return 1;
	}

	globox_ok = vlk_cmd_rec();

	if (globox_ok != 0)
	{
		printf("shit3\n");
		return 1;
	}

	vlk_semaphores();

#if 0
	globox_vlk(&ctx);
#endif

	globox_set_title(&ctx, "HELO");
	globox_set_icon(
		&ctx,
		(uint32_t*) &iconpix_beg,
		2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
	globox_commit(&ctx);

	while (!ctx.closed)
	{
		globox_prepoll(&ctx);

		// internal event dispatching by globox
		// `globox_poll_events` is an alternative
		if (!globox_wait_events(&ctx))
		{
			ctx.closed = true;
			break;
		}

		handler();
	}

	free(vlk_image_list);
	free(vlk_cmd_list);
	vkDestroySemaphore(vlk_dev_logical, vlk_present_semaphore, NULL);
	vkDestroySemaphore(vlk_dev_logical, vlk_render_semaphore, NULL);
	vkDestroySwapchainKHR(vlk_dev_logical, vlk_chain, NULL);
	vkDestroyCommandPool(vlk_dev_logical, vlk_cmd_pool, NULL);
	vkDestroyDevice(vlk_dev_logical, NULL);

	PFN_vkDestroyDebugReportCallbackEXT destroy =
		(PFN_vkDestroyDebugReportCallbackEXT)
		vkGetInstanceProcAddr(
			ctx.vlk_instance,
			"vkDestroyDebugReportCallbackEXT");

	if (destroy == NULL)
	{
		return 1;
	}

	destroy(
		ctx.vlk_instance,
		vlk_debug,
		NULL);

	globox_close(&ctx);

	return 0;
}
