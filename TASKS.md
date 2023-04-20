# Tasks

## GUI TODO
* [ ] Integrate tracy profiler
* [ ] Create animation debugger

## GUI Done
* [x] Implement FPS counter
* [x] ~Implement memory visualizer~

## Character movement with arrows demo
* [ ] Handle multiple input more gracefully
* [x] Sometimes, the horizontal animation flips get inverted. Why?
### Add dedicated background entity
* [x] Rename global get shader to get sprite shader
* [x] Add another shader in the window class
* [x] Use `glUseProgram(*sprite_shader);` whenever necessary, and not globally.

##  Main TODO
* [ ] Finish state changes demo
* [ ] Fix paths in scripts
* [ ] Add show/hide to debug windows triggered by customized key
* [ ] Investigate why hidden cursors are not working
* [ ] Add VM hooks for joysticks?
* [ ] Make shaders read SPIRV files (available in OpenGL 4.6)
* [ ] Find out safer way to provide VM index from the lua state.
* [ ] Add parallel task synchronization. Review parallel task system?
* [ ] Color lookup for animations [see here](https://www.youtube.com/watch?v=HsOKwUwL1bE)
* [ ] Make `actor` entity use `animated_sprite` entity (compose objects)

## Main Done
* [x] Review file loading to memory functions. Implement: load to buffer, ~load given allocator~
* [x] Implement do_file_at with more granularity
* [x] Implement safety net in lua_log_message with try catch.
* [x] Implement more log functions.
* [x] Find out the VM index from the lua state.
* [x] Review image loader
* [x] Create stack-backed static allocator.
* [x] Allow stack allocator to free out of order using the stack backed allocator.
* [x] Add VM hooks to load and drop images.
* [x] Review opengl buffer arrays
* [x] Load shaders in vm.
* [x] Add vm hooks for shaders: Compiling and using
* [x] Add static mesh drawing
* [x] Add texture to static mesh
* [x] Remake the static mesh as a single drawable sprite.
* [x] Remove global opengl buffers.
* [x] Create sprite shader.
* [x] Add sprite drawing to script.
* [x] Commit to a 2D renderer?.
* [x] Use sprite shader to shade all sprites. 
* [x] Animate sprites via scripts
* [x] Add script hooks for keyboard input
* [x] Thread allocators are not getting destroyed. Investigate.
* [x] Add VM hooks for parallel job system
* [x] Add script hooks for mouse input
* [x] Handle sprite sets with many sprite sheets of different properties
* [x] Finish background test demo
* [x] Add a function to reset quad geometry
* [x] Add a general sprite compiler
* [x] Read animation from .sad files
* [x] Add actor interface/abstraction in the engine
* [x] Add animation playing in actor abstraction
* [x] Add texture flipping.
* [x] Anchor point relative motion
* [x] Track actor heading.
* [x] Review allocator system.
* [x] Finish character animation with arrow movement demo
* [x] Fix hot reloading
* [x] Review Sprite API exposed to Lua