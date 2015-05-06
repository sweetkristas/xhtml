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
#include "filesystem.hpp"
#include "CameraObject.hpp"
#include "Canvas.hpp"
#include "Font.hpp"
#include "RenderManager.hpp"
#include "SceneGraph.hpp"
#include "SceneNode.hpp"
#include "SDLWrapper.hpp"
#include "WindowManager.hpp"
#include "profile_timer.hpp"
#include "variant_utils.hpp"
#include "unit_test.hpp"

#include "css_parser.hpp"
#include "display_list.hpp"
#include "font_freetype.hpp"
#include "xhtml.hpp"
#include "xhtml_layout.hpp"
#include "xhtml_node.hpp"
#include "xhtml_render_ctx.hpp"

void load_xhtml(int width, const std::string& ua_ss, const std::string& test_doc, xhtml::DisplayListPtr display_list)
{
	auto user_agent_style_sheet = std::make_shared<css::StyleSheet>();
	css::Parser::parse(user_agent_style_sheet, sys::read_file(ua_ss));

	auto doc_frag = xhtml::parse_from_file(test_doc);
	auto doc = xhtml::Document::create(user_agent_style_sheet);
	doc->addChild(doc_frag);
	//doc->normalize();
	doc->processStyles();
	// whitespace can only be processed after applying styles.
	doc->processWhitespace();

	//doc->preOrderTraversal([](xhtml::NodePtr n) {
	//	LOG_DEBUG(n->toString());
	//	return true;
	//});

	//xhtml::RenderContextManager rcm;
	// layout has to happen after initialisation of graphics
	auto layout = xhtml::Box::createLayout(doc, width);
	//layout->render(display_list, point());

	/*layout->preOrderTraversal([](xhtml::LayoutBoxPtr box, int nesting) {
		std::string indent(nesting*2, ' ');
		LOG_DEBUG(indent + box->toString());
	}, 0);*/
}

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

	if(!test::run_tests()) {
		// Just exit if some tests failed.
		exit(1);
	}

#if defined(__linux__)
	const std::string data_path = "data/";
#else
	const std::string data_path = "../data/";
#endif
	const std::string test_doc = data_path + "test5.xhtml";
	//const std::string test_doc = data_path + "storyboard.xhtml";
	const std::string ua_ss = data_path + "user_agent.css";

	sys::file_path_map font_files;
	sys::get_unique_files(data_path + "fonts/", font_files);
	KRE::FontDriver::setAvailableFonts(font_files);

#if 1
	WindowManager wm("SDL");

	variant_builder hints;
	hints.add("renderer", "opengl");
	hints.add("dpi_aware", true);
	hints.add("use_vsync", true);

	LOG_DEBUG("Creating window of size: " << width << "x" << height);
	auto main_wnd = wm.createWindow(width, height, hints.build());
	main_wnd->enableVsync(true);
	const float aspect_ratio = static_cast<float>(width) / height;

#if defined(__linux__)
	LOG_DEBUG("setting image file filter to 'images/'");
	Surface::setFileFilter(FileFilterType::LOAD, [](const std::string& fname) { return "images/" + fname; });
	Surface::setFileFilter(FileFilterType::SAVE, [](const std::string& fname) { return "images/" + fname; });
#else
	LOG_DEBUG("setting image file filter to '../images/'");
	Surface::setFileFilter(FileFilterType::LOAD, [](const std::string& fname) { return "../images/" + fname; });
	Surface::setFileFilter(FileFilterType::SAVE, [](const std::string& fname) { return "../images/" + fname; });
#endif
	Font::setAvailableFonts(font_files);

	SceneGraphPtr scene = SceneGraph::create("main");
	SceneNodePtr root = scene->getRootNode();
	root->setNodeName("root_node");

	DisplayDevice::getCurrent()->setDefaultCamera(std::make_shared<Camera>("ortho1", 0, width, 0, height));

	auto rman = std::make_shared<RenderManager>();
	auto rq = rman->addQueue(0, "opaques");

	xhtml::DisplayListPtr display_list = std::make_shared<xhtml::DisplayList>(scene);
	root->attachNode(display_list);
	load_xhtml(width, ua_ss, test_doc, display_list);

	auto canvas = Canvas::getInstance();

	auto txt = Font::getInstance()->renderText("Thequickbrownfoxjumpsoverthelazydog.", Color::colorWhite(), static_cast<int>(24.0*96.0/72.0), true, "FreeSerif.ttf");


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

		//Canvas::getInstance()->blitTexture(txt, 0, rect(0, 200));
	
		/*layout->preOrderTraversal([canvas](xhtml::LayoutBoxPtr box) {
			auto css_color = box->getNodeStyle("color");
			if(!css_color.empty()) {
				canvas->drawSolidRect(box->getContentDimensions().as_type<int>(), css_color.getValue<css::CssColor>().getColor());
			} else {
				canvas->drawSolidRect(box->getContentDimensions().as_type<int>(), Color::colorBlueviolet());
			}
		});*/

		// Called once a cycle before rendering.
		Uint32 current_tick_time = SDL_GetTicks();
		scene->process((current_tick_time - last_tick_time) / 1000.0f);
		last_tick_time = current_tick_time;

		scene->renderScene(rman);
		rman->render(main_wnd);

		main_wnd->swap();
	}
#endif

	return 0;
}
