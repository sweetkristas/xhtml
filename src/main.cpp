/*
	Copyright (C) 2003-2013 by Kristina Simpson <sweet.kristas@gmail.com>
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	   1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.

	   2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.

	   3. This notice may not be removed or altered from any source
	   distribution.
*/

#include "asserts.hpp"
#include "CameraObject.hpp"
#include "Font.hpp"
#include "RenderManager.hpp"
#include "SceneGraph.hpp"
#include "SceneNode.hpp"
#include "SDLWrapper.hpp"
#include "WindowManager.hpp"
#include "variant_utils.hpp"

#include "xhtml.hpp"

int main(int argc, char* argv[])
{
	std::vector<std::string> args;
	for(int i = 0; i != argc; ++i) {
		args.emplace_back(argv[i]);
	}

	int width = 800;
	int height = 600;

	using namespace KRE;
	SDL::SDL_ptr manager(new SDL::SDL());
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

	WindowManager wm("SDL");

	variant_builder hints;
	hints.add("renderer", "opengl");
	hints.add("dpi_aware", true);
	hints.add("use_vsync", true);

	LOG_DEBUG("Creating window of size: " << width << "x" << height);
	auto main_wnd = wm.createWindow(width, height, hints.build());
	main_wnd->enableVsync(true);
	const float aspect_ratio = static_cast<float>(width) / height;

	std::map<std::string, std::string> font_paths;
#if defined(__linux__)
	LOG_DEBUG("setting image file filter to 'images/'");
	Surface::setFileFilter(FileFilterType::LOAD, [](const std::string& fname) { return "images/" + fname; });
	Surface::setFileFilter(FileFilterType::SAVE, [](const std::string& fname) { return "images/" + fname; });
	
	font_paths["FreeSans.ttf"] = "data/fonts/FreeSans.ttf";
#else
	LOG_DEBUG("setting image file filter to '../images/'");
	Surface::setFileFilter(FileFilterType::LOAD, [](const std::string& fname) { return "../images/" + fname; });
	Surface::setFileFilter(FileFilterType::SAVE, [](const std::string& fname) { return "../images/" + fname; });

	font_paths["FreeSans.ttf"] = "../data/fonts/FreeSans.ttf";
#endif
	Font::setAvailableFonts(font_paths);

	SceneGraphPtr scene = SceneGraph::create("main");
	SceneNodePtr root = scene->getRootNode();
	root->setNodeName("root_node");

	DisplayDevice::getCurrent()->setDefaultCamera(std::make_shared<Camera>("ortho1", 0, width, 0, height));

	auto rman = std::make_shared<RenderManager>();
	auto rq = rman->addQueue(0, "opaques");

#if defined(__linux__)
	const std::string test_doc = "data/test1.xhtml";
#else
	const std::string test_doc = "../data/test1.xhtml";
#endif

	auto doc = xhtml::Parser::parseFromFile(test_doc);
	//auto doc_node = doc->render(scene, variant());
	//root->attachNode(doc_node);

	SDL_Event e;
	bool done = false;
	Uint32 last_tick_time = SDL_GetTicks();
	while(!done) {
		while(SDL_PollEvent(&e)) {
			// XXX we need to add some keyboard/mouse callback handling here for "doc".
			if(e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			} else if(e.type == SDL_KEYDOWN) {
				LOG_DEBUG("KEY PRESSED: " << SDL_GetKeyName(e.key.keysym.sym) << " : " << e.key.keysym.sym << " : " << e.key.keysym.scancode);
			} else if(e.type == SDL_QUIT) {
				done = true;
			}
		}

		main_wnd->clear(ClearFlags::ALL);

		// Called once a cycle before rendering.
		Uint32 current_tick_time = SDL_GetTicks();
		scene->process((current_tick_time - last_tick_time) / 1000.0f);
		last_tick_time = current_tick_time;

		scene->renderScene(rman);
		rman->render(main_wnd);

		main_wnd->swap();
	}

	return 0;
}
