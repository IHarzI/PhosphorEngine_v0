set "VulkanCompliePath=%VULKAN_SDK%\Bin\glslc.exe"
set "ShaderSrc = PH_EngineSource/shaders"

for %%i in (PH_EngineSource/shaders/*.vert PH_EngineSource/shaders/*.frag PH_EngineSource/shaders/*.comp ) do ("%VulkanCompliePath%" PH_EngineSource/shaders/%%i -o PH_EngineSource/shaders/%%i.spv)
for %%i in (PH_EngineSource/shaders/*.vert PH_EngineSource/shaders/*.frag PH_EngineSource/shaders/*.comp ) do ("%VulkanCompliePath%" PH_EngineSource/shaders/%%i -o Sandbox/shaders/%%i.spv)
