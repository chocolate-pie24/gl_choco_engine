https://raw.githubusercontent.com/chocolate-pie24/gl_choco_engine/refs/heads/<BRANCH_NAME>/<FILE_PATH>

BRANCH_NAME = develop
FILE_PATH

tree .
.
├── articles
│   └── c-glfw-game-engine-introduction.md
├── assets
│   ├── logo
│   │   ├── choco_engine_banner_768x256.png
│   │   ├── choco_engine_logo_256.png
│   │   ├── choco_engine_social_1280x640.png
│   │   └── original.png
│   ├── shaders
│   │   └── test_shader
│   │       ├── ui_shader.frag
│   │       └── ui_shader.vert
│   └── test
│       └── filesystem
│           ├── test_file.txt
│           └── test_file_w.txt
├── auto_zip.sh
├── books
│   ├── 2d_rendering_step1
│   │   ├── config.yaml
│   │   ├── cover.jpg
│   │   ├── cover.png
│   │   ├── step1_0_introduction.md
│   │   ├── step1_1_application_base.md
│   │   ├── step1_2_application_layer.md
│   │   ├── step1_3_base_layer.md
│   │   ├── step1_4_core_memory_linear_allocator.md
│   │   ├── step1_5_core_memory_system.md
│   │   └── step1_6_doxygen.md
│   ├── 2d_rendering_step2
│   │   ├── config.yaml
│   │   ├── cover.jpg
│   │   ├── cover.png
│   │   ├── step2_0_introduction.md
│   │   ├── step2_1_change_memory_system.md
│   │   ├── step2_2_change_linear_allocator.md
│   │   ├── step2_3_add_linux_support.md
│   │   ├── step2_4_add_container_string.md
│   │   ├── step2_5_add_platform_layer.md
│   │   └── step2_6_add_glfw_window.md
│   ├── 2d_rendering_step3
│   │   ├── config.yaml
│   │   ├── cover.jpg
│   │   ├── cover.png
│   │   ├── step3_0_introduction.md
│   │   ├── step3_1_event_system_abstract.md
│   │   ├── step3_2_ring_queue.md
│   │   ├── step3_3_event_pump_refactoring.md
│   │   ├── step3_4_mouse_event.md
│   │   └── step3_5_keyboard_event.md
│   └── 2d_rendering_step4
│       ├── appendix_buffer_explanation.md
│       ├── appendix_opengl_coordinates.md
│       ├── config.yaml
│       ├── cover.jpg
│       ├── cover.png
│       ├── step4_0_introduction.md
│       ├── step4_1_platform_layer.md
│       ├── step4_2_first_triangle.md
│       ├── step4_3_filesystem.md
│       ├── step4_4_renderer_backend.md
│       ├── step4_5_renderer_core.md
│       ├── step4_6_renderer_backend_interface.md
│       └── step4_7_renderer_backend_context.md
├── build.sh
├── coverage.sh
├── docs
│   ├── architecture
│   │   ├── platform_system
│   │   │   ├── architecture_en.md
│   │   │   ├── architecture_ja.md
│   │   │   ├── layer.mmd
│   │   │   ├── layer.png
│   │   │   ├── strategy.mmd
│   │   │   └── strategy.png
│   │   └── renderer_system
│   │       ├── layer.mmd
│   │       ├── layer.png
│   │       ├── renderer_backend
│   │       │   ├── architecture_en.md
│   │       │   ├── architecture_ja.md
│   │       │   ├── strategy.mmd
│   │       │   └── strategy.png
│   │       ├── renderer_system_en.md
│   │       └── renderer_system_ja.md
│   ├── doxygen
│   │   ├── doxygen_custom.css
│   │   ├── doxygen_icon_64.png
│   │   ├── doxygen_mainpage.md
│   │   ├── groups.dox
│   │   └── project_logo.png
│   ├── engine_overview.mmd
│   ├── engine_overview.png
│   ├── guide
│   │   ├── event_system
│   │   │   ├── dataflow.mmd
│   │   │   ├── dataflow.png
│   │   │   ├── event_en.md
│   │   │   └── event_ja.md
│   │   ├── platform_system
│   │   │   ├── adding_concretes_en.md
│   │   │   └── adding_concretes_ja.md
│   │   └── renderer_backend
│   │       ├── adding_concretes_en.md
│   │       └── adding_concretes_ja.md
│   └── layer.md
├── Doxyfile
├── images
│   ├── camera_coordinate.png
│   ├── ChatGPTRobot1.png
│   ├── ChatGPTRobot2.png
│   ├── clip_coordinate.png
│   ├── coordinate_pipeline.png
│   ├── coverage_book4.png
│   ├── event_system_diagram.png
│   ├── first_triangle.png
│   ├── local_coordinates.png
│   ├── log_example.png
│   ├── macOS_triangle.png
│   ├── memory_system_report.png
│   ├── pixel_coordinate.png
│   ├── ring_queue_memory_alignment.png
│   ├── ring_queue_pop.png
│   ├── ring_queue_push.png
│   └── world_coordinate.png
├── include
│   ├── application
│   │   └── application.h
│   └── engine
│       ├── base
│       │   ├── choco_macros.h
│       │   ├── choco_math
│       │   │   ├── choco_math.h
│       │   │   └── math_types.h
│       │   └── choco_message.h
│       ├── camera
│       │   └── camera.h
│       ├── containers
│       │   ├── choco_string.h
│       │   └── ring_queue.h
│       ├── core
│       │   ├── event
│       │   │   ├── keyboard_event.h
│       │   │   ├── mouse_event.h
│       │   │   └── window_event.h
│       │   ├── filesystem
│       │   │   └── filesystem.h
│       │   └── memory
│       │       ├── choco_memory.h
│       │       └── linear_allocator.h
│       ├── io_utils
│       │   └── fs_utils
│       │       └── fs_utils.h
│       ├── platform
│       │   ├── platform_concretes
│       │   │   └── platform_glfw.h
│       │   ├── platform_context.h
│       │   ├── platform_core
│       │   │   ├── platform_err_utils.h
│       │   │   └── platform_types.h
│       │   └── platform_interface.h
│       └── renderer
│           ├── renderer_backend
│           │   ├── renderer_backend_concretes
│           │   │   └── gl33
│           │   │       ├── concrete_shader.h
│           │   │       ├── concrete_vao.h
│           │   │       └── concrete_vbo.h
│           │   ├── renderer_backend_context
│           │   │   ├── context.h
│           │   │   ├── context_shader.h
│           │   │   ├── context_vao.h
│           │   │   └── context_vbo.h
│           │   ├── renderer_backend_interface
│           │   │   ├── interface_shader.h
│           │   │   ├── interface_vao.h
│           │   │   └── interface_vbo.h
│           │   └── renderer_backend_types.h
│           ├── renderer_core
│           │   ├── renderer_err_utils.h
│           │   ├── renderer_memory.h
│           │   └── renderer_types.h
│           └── renderer_resources
│               └── ui_shader.h
├── LICENSE
├── makefile_linux.mak
├── makefile_macos.mak
├── memo.md
├── README.md
├── REVIEW_FILES.md
├── sanitizer.sh
├── src
│   ├── application
│   │   └── application.c
│   ├── engine
│   │   ├── base
│   │   │   ├── choco_math
│   │   │   │   └── choco_math.c
│   │   │   └── choco_message.c
│   │   ├── camera
│   │   │   └── camera.c
│   │   ├── containers
│   │   │   ├── choco_string.c
│   │   │   └── ring_queue.c
│   │   ├── core
│   │   │   ├── filesystem
│   │   │   │   └── filesystem.c
│   │   │   └── memory
│   │   │       ├── choco_memory.c
│   │   │       └── linear_allocator.c
│   │   ├── io_utils
│   │   │   └── fs_utils
│   │   │       └── fs_utils.c
│   │   ├── platform
│   │   │   ├── platform_concretes
│   │   │   │   └── platform_glfw.c
│   │   │   ├── platform_context.c
│   │   │   └── platform_core
│   │   │       └── platform_err_utils.c
│   │   └── renderer
│   │       ├── renderer_backend
│   │       │   ├── renderer_backend_concretes
│   │       │   │   └── gl33
│   │       │   │       ├── concrete_shader.c
│   │       │   │       ├── concrete_vao.c
│   │       │   │       └── concrete_vbo.c
│   │       │   └── renderer_backend_context
│   │       │       └── context.c
│   │       ├── renderer_core
│   │       │   ├── renderer_err_utils.c
│   │       │   └── renderer_memory.c
│   │       └── renderer_resources
│   │           └── ui_shader.c
│   └── entry.c
├── test
│   ├── include
│   │   ├── platform
│   │   │   ├── test_platform_context.h
│   │   │   ├── test_platform_err_utils.h
│   │   │   └── test_platform_glfw.h
│   │   ├── renderer
│   │   │   ├── test_gl33_shader.h
│   │   │   ├── test_gl33_vao.h
│   │   │   ├── test_gl33_vbo.h
│   │   │   ├── test_renderer_backend_context.h
│   │   │   ├── test_renderer_err_utils.h
│   │   │   └── test_renderer_memory.h
│   │   ├── test_camera.h
│   │   ├── test_choco_math.h
│   │   ├── test_choco_string.h
│   │   ├── test_controller.h
│   │   ├── test_filesystem.h
│   │   ├── test_fs_utils.h
│   │   ├── test_linear_allocator.h
│   │   ├── test_memory_system.h
│   │   └── test_ring_queue.h
│   └── test_controller.c
├── todo.md
└── valgrind.sh
