/*
 * Copyright (C) 2003 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Main class for stellarium
// Manage all the objects to be used in the program

#include "stel_core.h"
#include "stellastro.h"
#include "stel_utility.h"

StelCore::StelCore(const string& CDIR, const string& LDIR, const string& DATA_ROOT) :
		skyTranslator(APP_NAME, LOCALEDIR, ""),
		projection(NULL), selected_object(NULL), hip_stars(NULL),
		nebulas(NULL), ssystem(NULL), milky_way(NULL), screen_W(800), screen_H(600), bppMode(16), Fullscreen(0),
		FlagHelp(false), FlagInfos(false), FlagConfig(false), FlagSearch(false), FlagShowTuiMenu(0),
		frame(0), timefr(0), timeBase(0), fps(0), maxfps(10000.f), deltaFov(0.), deltaAlt(0.), deltaAz(0.),
		move_speed(0.00025), FlagTimePause(0), is_mouse_moving_horiz(false), is_mouse_moving_vert(false)
{
	configDir = CDIR;
	localeDir = LDIR;
	dataRoot = DATA_ROOT;

	SelectedScript = SelectedScriptDirectory = "";

	projection = new Projector(800, 600, 60);
	tone_converter = new ToneReproductor();
	atmosphere = new Atmosphere();
	hip_stars = new HipStarMgr();
	asterisms = new ConstellationMgr(hip_stars);
	ssystem = new SolarSystem();
	observatory = new Observator(*ssystem);
	navigation = new Navigator(observatory);
	nebulas = new NebulaMgr();
	milky_way = new MilkyWay();
	equ_grid = new SkyGrid(SkyGrid::EQUATORIAL);
	azi_grid = new SkyGrid(SkyGrid::ALTAZIMUTAL);
	equator_line = new SkyLine(SkyLine::EQUATOR);
	ecliptic_line = new SkyLine(SkyLine::ECLIPTIC);
	meridian_line = new SkyLine(SkyLine::MERIDIAN, 1, 36);
	cardinals_points = new Cardinals();
	skyloc = new SkyLocalizer(getDataDir());

	ui = new StelUI(this);
	commander = new StelCommandInterface(this);
	scripts = new ScriptMgr(commander, getDataDir());
	script_images = new ImageMgr();

	time_multiplier = 1;
	object_pointer_visibility = 1;

	draw_mode = DM_NORMAL;
	ColorSchemeChanged = true;

	landscape = new LandscapeOldStyle();
	
	// Set textures directory and suffix
	s_texture::set_texDir(getDataRoot() + "/textures/");
	
}

StelCore::~StelCore()
{
	delete navigation;
	delete projection;
	delete asterisms;
	delete hip_stars;
	delete nebulas;
	delete equ_grid;
	delete azi_grid;
	delete equator_line;
	delete ecliptic_line;
	delete meridian_line;
	delete cardinals_points;
	delete landscape; landscape = NULL;
	delete observatory; observatory = NULL;
	delete skyloc; skyloc = NULL;
	delete milky_way;
	delete meteors; meteors = NULL;
	delete atmosphere;
	delete tone_converter;
	delete ssystem;
	delete ui;
	delete scripts;
	delete commander;
	delete script_images;

	StelObject::delete_textures(); // Unload the pointer textures
}

void StelCore::init(void)
{
	// Warning: These values are not overriden by the config!
	BaseCFontSize = 12.5;
	BaseCFontName = getDataDir() + "DejaVuSansMono-Roman.ttf";

	projection->set_fov(InitFov);

	// Init the solar system first
	ssystem->load(getDataDir() + "ssystem.ini");
	ssystem->setLabelColor(PlanetNamesColor[draw_mode]);
	ssystem->setOrbitColor(PlanetOrbitsColor[draw_mode]);
	ssystem->setFont(14.f, getDataDir() + BaseFontName);
	setPlanetsScale(getStarScale());
	ssystem->setTrailColor(ObjectTrailsColor[draw_mode]);

	observatory->load(configDir + config_file, "init_location");

	if (StartupTimeMode=="preset" || StartupTimeMode=="Preset")
		navigation->set_JDay(PresetSkyTime - observatory->get_GMT_shift(PresetSkyTime) * JD_HOUR);
	else navigation->set_JDay(get_julian_from_sys());
	navigation->set_local_vision(InitViewPos);

	// Load hipparcos stars & names
	LoadingBar lb(projection, 12., getDataDir() + BaseFontName, "logo24bits.png", screen_W, screen_H);
	hip_stars->init(
	    12.f, getDataDir() + BaseFontName,
	    getDataDir() + "hipparcos.fab",
	    getDataDir() + "star_names.fab",
	    getDataDir() + "name.fab",
	    lb);

	// Init nebulas
	nebulas->set_label_color(NebulaLabelColor[draw_mode]);
	nebulas->set_circle_color(NebulaCircleColor[draw_mode]);
	nebulas->read(12., getDataDir() + BaseFontName, getDataDir() + "ngc2000.dat", getDataDir() + "ngc2000names.dat", getDataDir() + "nebula_textures.fab", lb);
	nebulas->translateNames(skyTranslator);

	// Init stars
	hip_stars->set_label_color(StarLabelColor[draw_mode]);
	hip_stars->set_circle_color(StarCircleColor[draw_mode]);

	// Init grids, lines and cardinal points
	equ_grid->set_font(12., getDataDir() + BaseFontName);
	equ_grid->set_color(EquatorialColor[draw_mode]);

	azi_grid->set_font(12., getDataDir() + BaseFontName);
	azi_grid->set_color(AzimuthalColor[draw_mode]);

	equator_line->set_color(EquatorColor[draw_mode]);
	equator_line->set_font(12, getDataDir() + BaseFontName);

	ecliptic_line->set_color(EclipticColor[draw_mode]);
	ecliptic_line->set_font(12, getDataDir() + BaseFontName);

	meridian_line->set_color(AzimuthalColor[draw_mode]);
	meridian_line->set_font(12, getDataDir() + BaseFontName);

	cardinals_points->set_font(30., getDataDir() + BaseFontName);
	cardinals_points->set_color(CardinalColor[draw_mode]);

	// Init milky way
	if (draw_mode == DM_NORMAL)	milky_way->set_texture("milkyway.png");
	else milky_way->set_texture("milkyway_chart.png",true);

	milky_way->set_color(MilkyWayColor[draw_mode]);

	meteors = new MeteorMgr(10, 60);

	setLandscape(observatory->get_landscape_name());

	// Load the pointer textures
	StelObject::init_textures();

	// initialisation of the User Interface
	ui->init();
	ui->setTitleObservatoryName(ui->getTitleWithAltitude());
	ui->init_tui();

	if (!FlagNight) ui->desktop->setColorScheme(GuiBaseColor, GuiTextColor);
	else ui->desktop->setColorScheme(GuiBaseColorr, GuiTextColorr);

	// now redo this so we fill the autocomplete dialogs now UI inititalised
	// set_system_locale_by_name(SkyLocale); // and UILocale are the same but different format fra vs fr_FR!!!! TONY

	tone_converter->set_world_adaptation_luminance(3.75f + atmosphere->get_intensity()*40000.f);

	// Compute planets data and init viewing position
	// Position of sun and all the satellites (ie planets)
	ssystem->computePositions(navigation->get_JDay());
	// Matrix for sun and all the satellites (ie planets)
	ssystem->computeTransMatrices(navigation->get_JDay());

	// Compute transform matrices between coordinates systems
	navigation->update_transform_matrices();
	navigation->update_model_view_mat();

	// Load constellations
	string tmpstring=skyCulture; skyCulture=""; // Temporary trick
	setSkyCulture(tmpstring);

	asterisms->setLineColor(ConstLinesColor[draw_mode]);
	asterisms->setBoundaryColor(ConstBoundaryColor[draw_mode]);
	asterisms->setLabelColor(ConstNamesColor[draw_mode]);

	setPlanetsSelected("");	// Fix a bug on macosX! Thanks Fumio!
}

void StelCore::quit(void)
{
	static SDL_Event Q;						// Send a SDL_QUIT event
	Q.type = SDL_QUIT;						// To the SDL event queue
	if(SDL_PushEvent(&Q) == -1)				// Try to send the event
	{
		printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );
		exit(-1);
	}
}

// Update all the objects in function of the time
void StelCore::update(int delta_time)
{
	++frame;
	timefr+=delta_time;
	if (timefr-timeBase > 1000)
	{
		fps=frame*1000.0/(timefr-timeBase);				// Calc the FPS rate
		frame = 0;
		timeBase+=1000;
	}

	// change time rate if needed to fast forward scripts
	delta_time *= time_multiplier;


	// keep audio position updated if changing time multiplier
	if(!scripts->is_paused()) commander->update(delta_time);

	// run command from a running script
	scripts->update(delta_time);

	// Update the position of observation and time etc...
	observatory->update(delta_time);
	navigation->update_time(delta_time);

	// Position of sun and all the satellites (ie planets)
	ssystem->computePositions(navigation->get_JDay());
	// Matrix for sun and all the satellites (ie planets)
	ssystem->computeTransMatrices(navigation->get_JDay());


	// Transform matrices between coordinates systems
	navigation->update_transform_matrices();
	// Direction of vision
	navigation->update_vision_vector(delta_time, selected_object);
	// Field of view
	projection->update_auto_zoom(delta_time);

	// update faders and Planet trails (call after nav is updated)
	ssystem->update(delta_time, navigation);

	// Move the view direction and/or fov
	updateMove(delta_time);

	// Update info about selected object
	if (selected_object) selected_object->update();

	// Update faders
	equ_grid->update(delta_time);
	azi_grid->update(delta_time);
	equator_line->update(delta_time);
	ecliptic_line->update(delta_time);
	meridian_line->update(delta_time);
	asterisms->update(delta_time);
	atmosphere->update(delta_time);
	landscape->update(delta_time);
	hip_stars->update(delta_time);
	nebulas->update(delta_time);
	cardinals_points->update(delta_time);

	// Compute the sun position in local coordinate
	Vec3d temp(0.,0.,0.);
	Vec3d sunPos = navigation->helio_to_local(temp);


	// Compute the moon position in local coordinate
	temp = ssystem->getMoon()->get_heliocentric_ecliptic_pos();
	Vec3d moonPos = navigation->helio_to_local(temp);

	// Compute the atmosphere color and intensity
	atmosphere->compute_color(navigation->get_JDay(), sunPos, moonPos,
	                          ssystem->getMoon()->get_phase(ssystem->getEarth()->get_heliocentric_ecliptic_pos()),
	                          tone_converter, projection, observatory->get_latitude(), observatory->get_altitude(),
	                          15.f, 40.f);	// Temperature = 15c, relative humidity = 40%
	tone_converter->set_world_adaptation_luminance(atmosphere->get_world_adaptation_luminance());

	sunPos.normalize();
	moonPos.normalize();
	// compute global sky brightness TODO : make this more "scientifically"
	// TODO: also add moonlight illumination

	if(sunPos[2] < -0.1/1.5 )
	{
		sky_brightness = 0;
	}
	else
	{
		sky_brightness = (0.1 + 1.5*sunPos[2]) * atmosphere->get_intensity();
	}

	landscape->set_sky_brightness(sky_brightness);

	ui->gui_update_widgets(delta_time);

	//	if (FlagShowGravityUi || FlagShowTuiMenu) ui->tui_update_widgets();
	if (FlagShowTuiMenu) ui->tui_update_widgets();  // only update if in menu

	if(!scripts->is_paused()) script_images->update(delta_time);
}

// Execute all the drawing functions
void StelCore::draw(int delta_time)
{
	if (ColorSchemeChanged) ChangeColorScheme();

	// Init openGL viewing with fov, screen size and clip planes
	projection->set_clipping_planes(0.0005 ,50);

	// Give the updated standard projection matrices to the projector
	projection->set_modelview_matrices(	navigation->get_earth_equ_to_eye_mat(),
	                                    navigation->get_helio_to_eye_mat(),
	                                    navigation->get_local_to_eye_mat(),
	                                    navigation->get_j2000_to_eye_mat());

	// Set openGL drawings in equatorial coordinates
	navigation->switch_to_earth_equatorial();

	glBlendFunc(GL_ONE, GL_ONE);

	if (draw_mode != DM_NORMAL)
		draw_chart_background();

	// Draw the milky way.
	tone_converter->set_world_adaptation_luminance(atmosphere->get_milkyway_adaptation_luminance());
	if (draw_mode == DM_NORMAL)
		milky_way->draw(tone_converter, projection, navigation);
	else
		milky_way->draw_chart(tone_converter, projection, navigation);
	tone_converter->set_world_adaptation_luminance(atmosphere->get_world_adaptation_luminance());

	// Draw all the constellations
	asterisms->draw(projection, navigation);

	// Draw the nebula
	nebulas->draw(projection, navigation, tone_converter);

	// Draw the hipparcos stars
	Vec3d tempv = navigation->get_prec_equ_vision();
	Vec3f temp(tempv[0],tempv[1],tempv[2]);
	if (sky_brightness<=0.11)
	{
		hip_stars->draw(temp, tone_converter, projection);
	}

	// Draw the equatorial grid
	equ_grid->draw(projection);

	// Draw the altazimutal grid
	azi_grid->draw(projection);

	// Draw the celestial equator line
	equator_line->draw(projection);

	// Draw the ecliptic line
	ecliptic_line->draw(projection);

	// Draw the meridian line
	meridian_line->draw(projection);

	// Draw the pointer on the currently selected object
	if (selected_object && object_pointer_visibility) selected_object->draw_pointer(delta_time, projection, navigation);

	// Draw the planets
	ssystem->draw(projection, navigation, tone_converter, getFlagPointStar());

	// Set openGL drawings in local coordinates i.e. generally altazimuthal coordinates
	navigation->switch_to_local();

	// Draw meteors
	meteors->update(projection, navigation, tone_converter, delta_time);

	if(!getFlagAtmosphere() || sky_brightness<0.01)
	{
		projection->set_orthographic_projection();
		meteors->draw(projection, navigation);
		projection->reset_perspective_projection();
	}

	// Draw the atmosphere
	atmosphere->draw(projection, delta_time);

	// Draw the landscape
	landscape->draw(tone_converter, projection, navigation);

	// Draw the cardinal points
	//if (FlagCardinalPoints)
	cardinals_points->draw(projection, observatory->get_latitude());

	// draw images loaded by a script
	projection->set_orthographic_projection();
	script_images->draw(screen_W, screen_H, navigation, projection);

	projection->reset_perspective_projection();

	projection->draw_viewport_shape();

	// Draw the Graphical ui and the Text ui
	ui->draw();

	if (FlagShowGravityUi) ui->draw_gravity_ui();
	if (FlagShowTuiMenu) ui->draw_tui();

}


// Set the 2 config files names.
void StelCore::setConfigFiles(const string& _config_file)
{
	config_file = _config_file;
}


void StelCore::setLandscape(const string& new_landscape_name)
{
	if (new_landscape_name.empty()) return;
	Landscape* newLandscape = Landscape::create_from_file(getDataDir() + "landscapes.ini", new_landscape_name);
	if (landscape)
	{
		delete landscape;
		// Copy parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		landscape = newLandscape;
	}
	observatory->set_landscape_name(new_landscape_name);
}



void StelCore::setScreenSize(int w, int h)
{
	if (w==screen_W && h==screen_H) return;
	screen_W = w;
	screen_H = h;

	projection->set_screen_size(screen_W, screen_H);
}

// find and select the "nearest" object from earth equatorial position
StelObject * StelCore::find_stel_object(const Vec3d& v) const
{
	StelObject * sobj = NULL;

	if (getFlagPlanets()) sobj = ssystem->search(v, navigation, projection);
	if (sobj) return sobj;

	Vec3f u = navigation->earth_equ_to_j2000(v);

	//	Vec3f u=Vec3f(v[0],v[1],v[2]);

	sobj = nebulas->search(u);
	if (sobj) return sobj;

	if (getFlagStars()) sobj = hip_stars->search(u);

	return sobj;
}


// find and select the "nearest" object from screen position
StelObject * StelCore::find_stel_object(int x, int y) const
{
	Vec3d v;
	projection->unproject_earth_equ(x,y,v);
	return find_stel_object(v);
}

// Find and select in a "clever" way an object
StelObject * StelCore::clever_find(const Vec3d& v) const
{
	StelObject * sobj = NULL;
	vector<StelObject*> candidates;
	vector<StelObject*> temp;
	Vec3d winpos;

	// Field of view for a 30 pixel diameter circle on screen
	float fov_around = projection->get_fov()/MY_MIN(projection->viewW(), projection->viewH()) * 30.f;

	float xpos, ypos;
	projection->project_earth_equ(v, winpos);
	xpos = winpos[0];
	ypos = winpos[1];

	// Collect the planets inside the range
	if (getFlagPlanets())
	{
		temp = ssystem->search_around(v, fov_around, navigation, projection);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// nebulas and stars used precessed equ coords
	Vec3d p = navigation->earth_equ_to_j2000(v);

	// The nebulas inside the range
	if (getFlagNebula())
	{
		temp = nebulas->search_around(p, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// And the stars inside the range
	if (getFlagStars())
	{
		temp = hip_stars->search_around(p, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// Now select the object minimizing the function y = distance(in pixel) + magnitude
	float best_object_value;
	best_object_value = 100000.f;
	vector<StelObject*>::iterator iter = candidates.begin();
	while (iter != candidates.end())
	{
		projection->project_earth_equ((*iter)->get_earth_equ_pos(navigation), winpos);

		float distance = sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]));
		float mag = (*iter)->get_mag(navigation);
		if ((*iter)->get_type()==StelObject::STEL_OBJECT_NEBULA)
		{
			if( nebulas->getFlagHints() )
			{
				// make very easy to select if labeled
				mag = -1;
			}
		}
		if ((*iter)->get_type()==StelObject::STEL_OBJECT_PLANET)
		{
			if( getFlagPlanetsHints() )
			{
				// easy to select, especially pluto
				mag -= 15.f;
			}
			else
			{
				mag -= 8.f;
			}
		}
		if (distance + mag < best_object_value)
		{
			best_object_value = distance + mag;
			sobj = *iter;
		}
		iter++;
	}

	return sobj;
}

StelObject * StelCore::clever_find(int x, int y) const
{
	Vec3d v;
	projection->unproject_earth_equ(x,y,v);
	return clever_find(v);
}

// Go and zoom to the selected object.
void StelCore::autoZoomIn(float move_duration, bool allow_manual_zoom)
{
	float manual_move_duration;

	if (!selected_object) return;

	if (!navigation->get_flag_traking())
	{
		navigation->set_flag_traking(true);
		navigation->move_to(selected_object->get_earth_equ_pos(navigation), move_duration, false, 1);
		manual_move_duration = move_duration;
	}
	else
	{
		// faster zoom in manual zoom mode once object is centered
		manual_move_duration = move_duration*.66f;
	}

	if( allow_manual_zoom && FlagManualZoom )
	{
		// if manual zoom mode, user can zoom in incrementally
		float newfov = projection->get_fov()*0.5f;
		projection->zoom_to(newfov, manual_move_duration);

	}
	else
	{
		float satfov = selected_object->get_satellites_fov(navigation);
		float closefov = selected_object->get_close_fov(navigation);

		if (satfov>0. && projection->get_fov()*0.9>satfov) projection->zoom_to(satfov, move_duration);
		else if (projection->get_fov()>closefov) projection->zoom_to(closefov, move_duration);
	}
}

// Unzoom and go to the init position
void StelCore::autoZoomOut(float move_duration, bool full)
{
	if (!selected_object)
	{
		projection->zoom_to(InitFov, move_duration);
		navigation->move_to(InitViewPos, move_duration, true, -1);
		navigation->set_flag_traking(false);
		navigation->set_flag_lock_equ_pos(0);
		return;
	}

	// If the selected object has satellites, unzoom to satellites view unless specified otherwise
	if(!full)
	{
		float satfov = selected_object->get_satellites_fov(navigation);
		if (projection->get_fov()<=satfov*0.9 && satfov>0.)
		{
			projection->zoom_to(satfov, move_duration);
			return;
		}

		// If the selected object is part of a Planet subsystem (other than sun),
		// unzoom to subsystem view
		if (selected_object->get_type() == StelObject::STEL_OBJECT_PLANET && selected_object!=ssystem->getSun() && ((Planet*)selected_object)->get_parent()!=ssystem->getSun())
		{
			float satfov = ((Planet*)selected_object)->get_parent()->get_satellites_fov(navigation);
			if (projection->get_fov()<=satfov*0.9 && satfov>0.)
			{
				projection->zoom_to(satfov, move_duration);
				return;
			}
		}
	}

	projection->zoom_to(InitFov, move_duration);
	navigation->move_to(InitViewPos, move_duration, true, -1);
	navigation->set_flag_traking(false);
	navigation->set_flag_lock_equ_pos(0);
}

// this really belongs elsewhere
int StelCore::setSkyCulture(string _culture_dir)
{
	if(skyCulture == _culture_dir) return 2;

	// make sure culture definition exists before attempting
	if( !skyloc->test_sky_culture_directory(_culture_dir) )
	{
		cerr << "Invalid sky culture directory: " << _culture_dir << endl;
		return 0;
	}

	skyCulture = _culture_dir;

	if(!asterisms) return 3;

	LoadingBar lb(projection, 12., getDataDir() + BaseFontName, "logo24bits.png", screen_W, screen_H);

	asterisms->loadLinesAndArt(getDataDir() + "sky_cultures/" + skyCulture + "/constellationship.fab",
	                           getDataDir() + "sky_cultures/" + skyCulture + "/constellationsart.fab", getDataDir() + "sky_cultures/" + skyCulture + "/boundaries.dat", lb);
	asterisms->loadNames(getDataDir() + "sky_cultures/" + skyCulture + "/constellation_names.eng.fab");

	// Re-translated constellation names
	asterisms->translateNames(skyTranslator);

	// as constellations have changed, clear out any selection and retest for match!
	if (selected_object && selected_object->get_type()==StelObject::STEL_OBJECT_STAR)
	{
		asterisms->setSelected((HipStar*)selected_object);
	}
	else
	{
		asterisms->setSelected(NULL);
	}

	// update autocomplete with new names
	ui->setConstellationAutoComplete(asterisms->getNames());

	return 1;

}


//! @brief Set the application locale. This apply to GUI, console messages etc..
void StelCore::setAppLanguage(const std::string& newAppLocaleName)
{
	// Update the translator with new locale name
	Translator::globalTranslator = Translator(PACKAGE, LOCALEDIR, newAppLocaleName);
	cout << "Application locale is " << Translator::globalTranslator.getLocaleName() << endl;
}


//! @brief Set the sky locale and reload the sky objects names for gettext translation
void StelCore::setSkyLanguage(const std::string& newSkyLocaleName)
{
	if( !hip_stars || !cardinals_points || !asterisms) return; // objects not initialized yet

	// Update the translator with new locale name
	skyTranslator = Translator(PACKAGE, LOCALEDIR, newSkyLocaleName);
	cout << "Sky locale is " << skyTranslator.getLocaleName() << endl;

	// Translate all labels with the new language
	cardinals_points->translateLabels(skyTranslator);
	asterisms->translateNames(skyTranslator);
	ssystem->translateNames(skyTranslator);
	nebulas->translateNames(skyTranslator);
	
	// refresh EditBox with new names
	//ui->setStarAutoComplete(hip_stars->getNames());
	//ui->setConstellationAutoComplete(asterisms->getNames());
	//ui->setPlanetAutoComplete(ssystem->getNamesI18());
	//ui->setListNames(ssystem->getNamesI18());
}

void StelCore::playStartupScript()
{
	if(scripts) scripts->play_startup_script();
}

void StelCore::ChangeColorScheme(void)
{
	nebulas->set_label_color(NebulaLabelColor[draw_mode]);
	nebulas->set_circle_color(NebulaCircleColor[draw_mode]);
	hip_stars->set_label_color(StarLabelColor[draw_mode]);
	hip_stars->set_circle_color(StarCircleColor[draw_mode]);
	ssystem->setLabelColor(PlanetNamesColor[draw_mode]);
	ssystem->setOrbitColor(PlanetOrbitsColor[draw_mode]);
	ssystem->setTrailColor(ObjectTrailsColor[draw_mode]);
	equ_grid->set_color(EquatorialColor[draw_mode]);
	equ_grid->set_top_transparancy(draw_mode==DM_NORMAL);
	azi_grid->set_color(AzimuthalColor[draw_mode]);
	azi_grid->set_top_transparancy(draw_mode==DM_NORMAL);
	equator_line->set_color(EquatorColor[draw_mode]);
	ecliptic_line->set_color(EclipticColor[draw_mode]);
	meridian_line->set_font(12, getDataDir() + BaseFontName);
	cardinals_points->set_color(CardinalColor[draw_mode]);

	// Init milky way
	if (draw_mode == DM_NORMAL)	milky_way->set_texture("milkyway.png");
	else milky_way->set_texture("milkyway_chart.png",true);

	milky_way->set_color(MilkyWayColor[draw_mode]);
	asterisms->setLineColor(ConstLinesColor[draw_mode]);
	asterisms->setBoundaryColor(ConstBoundaryColor[draw_mode]);
	asterisms->setLabelColor(ConstNamesColor[draw_mode]);

	ColorSchemeChanged = false;
}

void StelCore::draw_chart_background(void)
{
	int stepX = projection->viewW();
	int stepY = projection->viewH();
	int viewport_left = projection->view_left();
	int view_bottom = projection->view_bottom();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor3fv(ChartColor[draw_mode]);
	projection->set_orthographic_projection();	// set 2D coordinate
	glBegin(GL_QUADS);
	glTexCoord2s(0, 0); glVertex2i(viewport_left, view_bottom);	// Bottom Left
	glTexCoord2s(1, 0); glVertex2i(viewport_left+stepX, view_bottom);	// Bottom Right
	glTexCoord2s(1, 1); glVertex2i(viewport_left+stepX,view_bottom+stepY);	// Top Right
	glTexCoord2s(0, 1); glVertex2i(viewport_left,view_bottom+stepY);	// Top Left
	glEnd();
	projection->reset_perspective_projection();
}

wstring StelCore::get_cursor_pos(int x, int y)
{
	Vec3d v;
	projection->unproject_earth_equ(x,y,v);

	wostringstream oss;

	float tempDE, tempRA;
	rect_to_sphe(&tempRA,&tempDE,v);
	oss << "RA : " << StelUtility::printAngleHMS(tempRA) << endl;
	oss << "DE : " << StelUtility::printAngleDMS(tempDE);

	return oss.str();
}

void StelCore::setProjectionType(Projector::PROJECTOR_TYPE pType)
{
	if (getProjectionType()==pType) return;
	Projector* ptemp;

	switch (pType)
	{
	case Projector::PERSPECTIVE_PROJECTOR :
		ptemp = new Projector(projection->get_screenW(), projection->get_screenH(), projection->get_fov());
		break;
	case Projector::FISHEYE_PROJECTOR :
		ptemp = new FisheyeProjector(projection->get_screenW(), projection->get_screenH(), projection->get_fov());
		break;
	case Projector::CYLINDER_PROJECTOR :
		ptemp = new CylinderProjector(projection->get_screenW(), projection->get_screenH(), projection->get_fov());
		break;
	default :
		assert(0);	// This should never happen
	}
	ptemp->setViewportType(projection->getViewportType());
	ptemp->setViewportHorizontalOffset(projection->getViewportHorizontalOffset());
	ptemp->setViewportVerticalOffset(projection->getViewportVerticalOffset());
	ptemp->setFlagGravityLabels(projection->getFlagGravityLabels());
	delete projection;
	projection = ptemp;
}

void StelCore::loadConfig(void)
{
	InitParser conf;
	conf.load(configDir + config_file);

	// Main section
	string version = conf.get_str("main:version");
	if (version!=string(VERSION))
	{
		if (version>="0.6.0" && version != string(VERSION))
		{
			cout << "The current config file is from a previous version (>=0.6.0).\nPrevious options will be imported in the new config file." << endl;

			// Store temporarily the previous observator parameters
			Observator tempobs(*ssystem);
			tempobs.load(configDir + config_file, "init_location");

			// Set the new landscape though
			tempobs.set_landscape_name("Guereins");

			loadConfigFrom(configDir + config_file);
			// We just imported previous parameters (from >=0.6.0)
			saveConfigTo(configDir + config_file);
			tempobs.save(configDir + config_file, "init_location");

			loadConfigFrom(configDir + config_file);
		}
		else
		{
			// The config file is too old to try an importation
			printf("The current config file is from a version too old for parameters to be imported (%s).\nIt will be replaced by the default config file.\n", version.empty() ? "<0.6.0" : version.c_str());
			system( (string("cp -f ") + dataRoot + "/config/default_config.ini " + configDir + config_file).c_str() );

			// Actually load the config file
			loadConfigFrom(configDir + config_file);
			return;
		}
	}

	// Versions match, there was no pblms
	loadConfigFrom(configDir + config_file);

}

void StelCore::saveConfig(void)
{
	// The config file is supposed to be valid and from the correct stellarium version.
	// This is normally the case if the program is running.
	saveConfigTo(configDir + config_file);
}

void StelCore::loadConfigFrom(const string& confFile)
{
	cout << "Loading configuration file " << confFile << " ..." << endl;
	InitParser conf;
	conf.load(confFile);

	// Main section (check for version mismatch)
	string version = conf.get_str("main:version");
	if (version!=string(VERSION) && version<"0.6.0")
	{
		cerr << "ERROR : The current config file is from a different version (" <<
		(version.empty() ? "<0.6.0" : version) << ")." << endl;
		exit(-1);
	}

	// Video Section
	Fullscreen			= conf.get_boolean("video:fullscreen");
	setScreenSize(conf.get_int("video:screen_w"), conf.get_int("video:screen_h"));
	bppMode				= conf.get_int    ("video:bbp_mode");
	setViewportHorizontalOffset(conf.get_int    ("video:horizontal_offset"));
	setViewportVerticalOffset(conf.get_int    ("video:vertical_offset"));
	maxfps 				= conf.get_double ("video","maximum_fps",10000);

	// Projector
	string tmpstr = conf.get_str("projection:type");
	Projector::PROJECTOR_TYPE projType;
	if (tmpstr=="perspective") projType = Projector::PERSPECTIVE_PROJECTOR;
	else
	{
		if (tmpstr=="fisheye") projType = Projector::FISHEYE_PROJECTOR;
		else
		{
			if (tmpstr=="cylinder") projType = Projector::CYLINDER_PROJECTOR;
			else
			{
				cerr << "ERROR : Unknown projector type : " << tmpstr << endl;
				exit(-1);
			}
		}
	}
	setProjectionType(projType);

	tmpstr = conf.get_str("projection:viewport");
	Projector::VIEWPORT_TYPE viewType;
	if (tmpstr=="maximized") viewType = Projector::MAXIMIZED;
	else
		if (tmpstr=="square") viewType = Projector::SQUARE;
		else
		{
			if (tmpstr=="disk") viewType = Projector::DISK;
			else
			{
				cerr << "ERROR : Unknown viewport type : " << tmpstr << endl;
				exit(-1);
			}
		}
	setViewportType(viewType);

	// localization section
	skyCulture = conf.get_str("localization", "sky_culture", "western");

	string skyLocaleName = conf.get_str("localization", "sky_locale", "system");
	string appLocaleName = conf.get_str("localization", "app_locale", "system");
	setSkyLanguage(skyLocaleName);
	setAppLanguage(appLocaleName);

	// Star section
	setStarScale(conf.get_double ("stars:star_scale"));
	setPlanetsScale(conf.get_double ("stars:star_scale"));  // if reload config

	setStarMagScale(conf.get_double ("stars:star_mag_scale"));
	setStarTwinkleAmount(conf.get_double ("stars:star_twinkle_amount"));
	setMaxMagStarName(conf.get_double ("stars:max_mag_star_name"));
	setFlagStarTwinkle(conf.get_boolean("stars:flag_star_twinkle"));
	setFlagPointStar(conf.get_boolean("stars:flag_point_star"));
	setStarLimitingMag(conf.get_double("stars", "star_limiting_mag", 6.5f));

	// Ui section
	FlagShowFps			= conf.get_boolean("gui:flag_show_fps");
	FlagMenu			= conf.get_boolean("gui:flag_menu");
	FlagHelp			= conf.get_boolean("gui:flag_help");
	FlagInfos			= conf.get_boolean("gui:flag_infos");
	FlagShowTopBar		= conf.get_boolean("gui:flag_show_topbar");
	FlagShowTime		= conf.get_boolean("gui:flag_show_time");
	FlagShowDate		= conf.get_boolean("gui:flag_show_date");
	FlagShowAppName		= conf.get_boolean("gui:flag_show_appname");
	FlagShowFov			= conf.get_boolean("gui:flag_show_fov");
	FlagShowSelectedObjectInfo = conf.get_boolean("gui:flag_show_selected_object_info");
	GuiBaseColor		= StelUtility::str_to_vec3f(conf.get_str("color", "gui:gui_base_color", "0.3,0.4,0.7").c_str());
	GuiTextColor		= StelUtility::str_to_vec3f(conf.get_str("color", "gui:gui_text_color", "0.7,0.8,0.9").c_str());
	GuiBaseColorr		= StelUtility::str_to_vec3f(conf.get_str("color", "gui:gui_base_colorr", "0.7,0.2,0.1").c_str());
	GuiTextColorr		= StelUtility::str_to_vec3f(conf.get_str("color", "gui:gui_text_colorr", "0.9,0.4,0.2").c_str());
	BaseFontSize		= conf.get_double ("gui","base_font_size",15);
	BaseFontName        = conf.get_str("gui", "base_font_name", "DejaVuSans.ttf");
	FlagShowScriptBar	= conf.get_boolean("gui","flag_show_script_bar",0);
	MouseCursorTimeout  = conf.get_double("gui","mouse_cursor_timeout",0);
	scripts->set_allow_ui( conf.get_boolean("gui","flag_script_allow_ui",0) );

	// Colors
	AzimuthalColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:azimuthal_color").c_str());
	EquatorialColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:equatorial_color").c_str());
	EquatorColor[0]			= StelUtility::str_to_vec3f(conf.get_str("color:equator_color").c_str());
	EclipticColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:ecliptic_color").c_str());
	ConstLinesColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:const_lines_color").c_str());
	ConstNamesColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:const_names_color").c_str());
	ConstBoundaryColor[0]	= StelUtility::str_to_vec3f(conf.get_str("color", "const_boundary_color", "0.8,0.3,0.3").c_str());
	NebulaLabelColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:nebula_label_color").c_str());
	NebulaCircleColor[0]	= StelUtility::str_to_vec3f(conf.get_str("color:nebula_circle_color").c_str());
	StarLabelColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:star_label_color").c_str());
	StarCircleColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:star_circle_color").c_str());
	CardinalColor[0] 		= StelUtility::str_to_vec3f(conf.get_str("color:cardinal_color").c_str());
	PlanetNamesColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:planet_names_color").c_str());
	PlanetOrbitsColor[0]	= StelUtility::str_to_vec3f(conf.get_str("color", "planet_orbits_color", ".6,1,1").c_str());
	ObjectTrailsColor[0]	= StelUtility::str_to_vec3f(conf.get_str("color", "object_trails_color", "1,0.7,0").c_str());
	ChartColor[0]			= StelUtility::str_to_vec3f(conf.get_str("color:chart_color").c_str());
	MilkyWayColor[0]		= StelUtility::str_to_vec3f(conf.get_str("color:milky_way_color").c_str());

	AzimuthalColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:azimuthal_color").c_str());
	EquatorialColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:equatorial_color").c_str());
	EquatorColor[1]			= StelUtility::str_to_vec3f(conf.get_str("colorc:equator_color").c_str());
	EclipticColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:ecliptic_color").c_str());
	ConstLinesColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:const_lines_color").c_str());
	ConstBoundaryColor[1]	= StelUtility::str_to_vec3f(conf.get_str("colorc", "const_boundary_color", "0.8,0.3,0.3").c_str());
	ConstNamesColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:const_names_color").c_str());
	NebulaLabelColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:nebula_label_color").c_str());
	NebulaCircleColor[1]	= StelUtility::str_to_vec3f(conf.get_str("colorc:nebula_circle_color").c_str());
	StarLabelColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:star_label_color").c_str());
	StarCircleColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:star_circle_color").c_str());
	CardinalColor[1] 		= StelUtility::str_to_vec3f(conf.get_str("colorc:cardinal_color").c_str());
	PlanetNamesColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:planet_names_color").c_str());
	PlanetOrbitsColor[1]	= StelUtility::str_to_vec3f(conf.get_str("colorc", "planet_orbits_color", ".6,1,1").c_str());
	ObjectTrailsColor[1]	= StelUtility::str_to_vec3f(conf.get_str("colorc", "object_trails_color", "1,0.7,0").c_str());
	ChartColor[1]			= StelUtility::str_to_vec3f(conf.get_str("colorc:chart_color").c_str());
	MilkyWayColor[1]		= StelUtility::str_to_vec3f(conf.get_str("colorc:milky_way_color").c_str());

	AzimuthalColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:azimuthal_color").c_str());
	EquatorialColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:equatorial_color").c_str());
	EquatorColor[2]			= StelUtility::str_to_vec3f(conf.get_str("colorr:equator_color").c_str());
	EclipticColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:ecliptic_color").c_str());
	ConstLinesColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:const_lines_color").c_str());
	ConstBoundaryColor[2]	= StelUtility::str_to_vec3f(conf.get_str("colorr", "const_boundary_color", "0.8,0.3,0.3").c_str());
	ConstNamesColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:const_names_color").c_str());
	NebulaLabelColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:nebula_label_color").c_str());
	NebulaCircleColor[2]	= StelUtility::str_to_vec3f(conf.get_str("colorr:nebula_circle_color").c_str());
	StarLabelColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:star_label_color").c_str());
	StarCircleColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:star_circle_color").c_str());
	CardinalColor[2] 		= StelUtility::str_to_vec3f(conf.get_str("colorr:cardinal_color").c_str());
	PlanetNamesColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:planet_names_color").c_str());
	PlanetOrbitsColor[2]	= StelUtility::str_to_vec3f(conf.get_str("colorr", "planet_orbits_color", ".6,1,1").c_str());
	ObjectTrailsColor[2]	= StelUtility::str_to_vec3f(conf.get_str("colorr", "object_trails_color", "1,0.7,0").c_str());
	ChartColor[2]			= StelUtility::str_to_vec3f(conf.get_str("colorr:chart_color").c_str());
	MilkyWayColor[2]		= StelUtility::str_to_vec3f(conf.get_str("colorr:milky_way_color").c_str());

	// Text ui section
	FlagEnableTuiMenu = conf.get_boolean("tui:flag_enable_tui_menu");
	FlagShowGravityUi = conf.get_boolean("tui:flag_show_gravity_ui");
	FlagShowTuiDateTime = conf.get_boolean("tui:flag_show_tui_datetime");
	FlagShowTuiShortObjInfo = conf.get_boolean("tui:flag_show_tui_short_obj_info");

	// Navigation section
	PresetSkyTime 		= conf.get_double ("navigation","preset_sky_time",2451545.);
	StartupTimeMode 	= conf.get_str("navigation:startup_time_mode");	// Can be "now" or "preset"
	FlagEnableZoomKeys	= conf.get_boolean("navigation:flag_enable_zoom_keys");
	FlagEnableMoveKeys  = conf.get_boolean("navigation:flag_enable_move_keys");
	FlagManualZoom		= conf.get_boolean("navigation:flag_manual_zoom");
	FlagEnableMoveMouse	= conf.get_boolean("navigation","flag_enable_move_mouse",1);
	InitFov				= conf.get_double ("navigation","init_fov",60.);
	InitViewPos 		= StelUtility::str_to_vec3f(conf.get_str("navigation:init_view_pos").c_str());
	auto_move_duration	= conf.get_double ("navigation","auto_move_duration",1.5);
	FlagUTC_Time		= conf.get_boolean("navigation:flag_utc_time");
	MouseZoom			= conf.get_int("navigation","mouse_zoom",30);
	move_speed			= conf.get_double("navigation","move_speed",0.0004);
	zoom_speed			= conf.get_double("navigation","zoom_speed", 0.0004);

	// Viewing Mode
	tmpstr = conf.get_str("navigation:viewing_mode");
	if (tmpstr=="equator") 	navigation->set_viewing_mode(Navigator::VIEW_EQUATOR);
	else
	{
		if (tmpstr=="horizon") navigation->set_viewing_mode(Navigator::VIEW_HORIZON);
		else
		{
			cerr << "ERROR : Unknown viewing mode type : " << tmpstr << endl;
			assert(0);
		}
	}

	// Landscape section
	setFlagLandscape(conf.get_boolean("landscape", "flag_landscape", conf.get_boolean("landscape", "flag_ground", 1)));  // name change
	setFlagFog(conf.get_boolean("landscape:flag_fog"));
	setFlagAtmosphere(conf.get_boolean("landscape:flag_atmosphere"));
	setAtmosphereFadeDuration(conf.get_double("landscape","atmosphere_fade_duration",1.5));

	// Viewing section
	setFlagConstellationLines(		conf.get_boolean("viewing:flag_constellation_drawing"));
	setFlagConstellationNames(		conf.get_boolean("viewing:flag_constellation_name"));
	setFlagConstellationBoundaries(	conf.get_boolean("viewing","flag_constellation_boundaries",false));
	setFlagConstellationArt(		conf.get_boolean("viewing:flag_constellation_art"));
	setFlagConstellationIsolateSelected(conf.get_boolean("viewing", "flag_constellation_isolate_selected",conf.get_boolean("viewing", "flag_constellation_pick", 0)));
	setConstellationArtIntensity(conf.get_double("viewing","constellation_art_intensity", 0.5));
	setConstellationArtFadeDuration(conf.get_double("viewing","constellation_art_fade_duration",2.));
	setConstellationFontSize(conf.get_double("viewing","constellation_font_size",16.));

	setFlagAzimutalGrid(conf.get_boolean("viewing:flag_azimutal_grid"));
	setFlagEquatorGrid(conf.get_boolean("viewing:flag_equatorial_grid"));
	setFlagEquatorLine(conf.get_boolean("viewing:flag_equator_line"));
	setFlagEclipticLine(conf.get_boolean("viewing:flag_ecliptic_line"));
	setFlagMeridianLine(conf.get_boolean("viewing:flag_meridian_line"));
	cardinals_points->setFlagShow(conf.get_boolean("viewing:flag_cardinal_points"));
	setFlagGravityLabels( conf.get_boolean("viewing:flag_gravity_labels") );
	// TODO uncomment when the main initialization is cleaned
	//setFlagMoonScaled(conf.get_boolean("viewing", "flag_moon_scaled",
	 //                                   conf.get_boolean("viewing", "flag_init_moon_scaled", 0)));  // name change
	setMoonScale(conf.get_double ("viewing","moon_scale",5.));
	FlagChart    			= conf.get_boolean("viewing:flag_chart");
	FlagNight    			= conf.get_boolean("viewing:flag_night");
	SetDrawMode();

	// Astro section
	setFlagStars(conf.get_boolean("astro:flag_stars"));
	setFlagStarName(conf.get_boolean("astro:flag_star_name"));
	setFlagPlanets(conf.get_boolean("astro:flag_planets"));
	setFlagPlanetsHints(conf.get_boolean("astro:flag_planets_hints"));
	setFlagPlanetsOrbits(conf.get_boolean("astro:flag_planets_orbits"));
	setFlagPlanetsTrails(conf.get_boolean("astro", "flag_object_trails", 0));
	startPlanetsTrails(conf.get_boolean("astro", "flag_object_trails", 0));
	setFlagNebula(conf.get_boolean("astro:flag_nebula"));
	setFlagNebulaHints(conf.get_boolean("astro:flag_nebula_name"));
	setNebulaMaxMagHints(conf.get_double("astro", "max_mag_nebula_name", 99));
	setNebulaCircleScale(conf.get_double("astro", "nebula_scale",1.0f));
	setFlagMilkyWay(conf.get_boolean("astro:flag_milky_way"));
	setMilkyWayIntensity(conf.get_double("astro","milky_way_intensity",1.));
	setFlagNebulaBright(conf.get_boolean("astro:flag_bright_nebulae"));
}

void StelCore::saveConfigTo(const string& confFile)
{
	cout << "Saving configuration file " << confFile << " ..." << endl;
	InitParser conf;

	// Main section
	conf.set_str	("main:version", string(VERSION));

	// Video Section
	conf.set_boolean("video:fullscreen", Fullscreen);
	conf.set_int	("video:screen_w", screen_W);
	conf.set_int	("video:screen_h", screen_H);
	conf.set_int	("video:bbp_mode", bppMode);
	conf.set_int    ("video:horizontal_offset", getViewportHorizontalOffset());
	conf.set_int    ("video:vertical_offset", getViewportVerticalOffset());
	conf.set_double ("video:maximum_fps", maxfps);

	// Projector
	string tmpstr;
	switch (getProjectionType())
	{
	case Projector::PERSPECTIVE_PROJECTOR : tmpstr="perspective";	break;
	case Projector::FISHEYE_PROJECTOR : tmpstr="fisheye";		break;
	case Projector::CYLINDER_PROJECTOR : tmpstr="cylinder";		break;
	default : tmpstr="perspective";
	}
	conf.set_str	("projection:type",tmpstr);

	switch (getViewportType())
	{
	case Projector::MAXIMIZED : tmpstr="maximized";	break;
	case Projector::SQUARE : tmpstr="square";	break;
	case Projector::DISK : tmpstr="disk";		break;
	default : tmpstr="maximized";
	}
	conf.set_str	("projection:viewport", tmpstr);

	// localization section
	conf.set_str    ("localization:sky_culture", skyCulture);
	conf.set_str    ("localization:app_locale", Translator::globalTranslator.getLocaleName());
	conf.set_str    ("localization:sky_locale", skyTranslator.getLocaleName());

	// Star section
	conf.set_double ("stars:star_scale", getStarScale());
	conf.set_double ("stars:star_mag_scale", getStarMagScale());
	conf.set_double ("stars:star_twinkle_amount", getStarTwinkleAmount());
	conf.set_double ("stars:max_mag_star_name", getMaxMagStarName());
	conf.set_boolean("stars:flag_star_twinkle", getFlagStarTwinkle());
	conf.set_boolean("stars:flag_point_star", getFlagPointStar());
	//	conf.set_double("stars:star_limiting_mag", hip_stars->get_limiting_mag());

	// Ui section
	conf.set_boolean("gui:flag_show_fps" ,FlagShowFps);
	conf.set_boolean("gui:flag_menu", FlagMenu);
	conf.set_boolean("gui:flag_help", FlagHelp);
	conf.set_boolean("gui:flag_infos", FlagInfos);
	conf.set_boolean("gui:flag_show_topbar", FlagShowTopBar);
	conf.set_boolean("gui:flag_show_time", FlagShowTime);
	conf.set_boolean("gui:flag_show_date", FlagShowDate);
	conf.set_boolean("gui:flag_show_appname", FlagShowAppName);
	conf.set_boolean("gui:flag_show_fov", FlagShowFov);
	conf.set_boolean("gui:flag_show_selected_object_info", FlagShowSelectedObjectInfo);
	conf.set_str	("gui:gui_base_color", StelUtility::vec3f_to_str(GuiBaseColor));
	conf.set_str	("gui:gui_text_color", StelUtility::vec3f_to_str(GuiTextColor));
	conf.set_str	("gui:gui_base_colorr", StelUtility::vec3f_to_str(GuiBaseColorr));
	conf.set_str	("gui:gui_text_colorr", StelUtility::vec3f_to_str(GuiTextColorr));
	conf.set_double ("gui:base_font_size", BaseFontSize);
	conf.set_str	("gui:base_font_name", BaseFontName);
	conf.set_boolean("gui:flag_show_script_bar",FlagShowScriptBar);
	conf.set_double("gui:mouse_cursor_timeout",MouseCursorTimeout);
	conf.set_boolean("gui:flag_script_allow_ui",scripts->get_allow_ui());

	// Colors
	conf.set_str    ("color:azimuthal_color", StelUtility::vec3f_to_str(AzimuthalColor[0]));
	conf.set_str    ("color:equatorial_color", StelUtility::vec3f_to_str(EquatorialColor[0]));
	conf.set_str    ("color:equator_color", StelUtility::vec3f_to_str(EquatorColor[0]));
	conf.set_str    ("color:ecliptic_color", StelUtility::vec3f_to_str(EclipticColor[0]));
	conf.set_str    ("color:const_lines_color", StelUtility::vec3f_to_str(ConstLinesColor[0]));
	conf.set_str    ("color:const_names_color", StelUtility::vec3f_to_str(ConstNamesColor[0]));
	conf.set_str    ("color:const_boundary_color", StelUtility::vec3f_to_str(ConstBoundaryColor[0]));
	conf.set_str	("color:nebula_label_color", StelUtility::vec3f_to_str(NebulaLabelColor[0]));
	conf.set_str	("color:nebula_circle_color", StelUtility::vec3f_to_str(NebulaCircleColor[0]));
	conf.set_str	("color:star_label_color", StelUtility::vec3f_to_str(StarLabelColor[0]));
	conf.set_str	("color:star_circle_color", StelUtility::vec3f_to_str(StarCircleColor[0]));
	conf.set_str    ("color:cardinal_color", StelUtility::vec3f_to_str(CardinalColor[0]));
	conf.set_str    ("color:planet_names_color", StelUtility::vec3f_to_str(PlanetNamesColor[0]));
	conf.set_str    ("color:planet_orbits_color", StelUtility::vec3f_to_str(PlanetOrbitsColor[0]));
	conf.set_str    ("color:object_trails_color", StelUtility::vec3f_to_str(ObjectTrailsColor[0]));
	conf.set_str    ("color:chart_color", StelUtility::vec3f_to_str(ChartColor[0]));

	conf.set_str    ("colorc:azimuthal_color", StelUtility::vec3f_to_str(AzimuthalColor[1]));
	conf.set_str    ("colorc:equatorial_color", StelUtility::vec3f_to_str(EquatorialColor[1]));
	conf.set_str    ("colorc:equator_color", StelUtility::vec3f_to_str(EquatorColor[1]));
	conf.set_str    ("colorc:ecliptic_color", StelUtility::vec3f_to_str(EclipticColor[1]));
	conf.set_str    ("colorc:const_lines_color", StelUtility::vec3f_to_str(ConstLinesColor[1]));
	conf.set_str    ("colorc:const_names_color", StelUtility::vec3f_to_str(ConstNamesColor[1]));
	conf.set_str    ("colorc:const_boundary_color", StelUtility::vec3f_to_str(ConstBoundaryColor[1]));
	conf.set_str	("colorc:nebula_label_color", StelUtility::vec3f_to_str(NebulaLabelColor[1]));
	conf.set_str	("colorc:nebula_circle_color", StelUtility::vec3f_to_str(NebulaCircleColor[1]));
	conf.set_str	("colorc:star_label_color", StelUtility::vec3f_to_str(StarLabelColor[1]));
	conf.set_str	("colorc:star_circle_color", StelUtility::vec3f_to_str(StarCircleColor[1]));
	conf.set_str    ("colorc:cardinal_color", StelUtility::vec3f_to_str(CardinalColor[1]));
	conf.set_str    ("colorc:planet_names_color", StelUtility::vec3f_to_str(PlanetNamesColor[1]));
	conf.set_str    ("colorc:planet_orbits_color", StelUtility::vec3f_to_str(PlanetOrbitsColor[1]));
	conf.set_str    ("colorc:object_trails_color", StelUtility::vec3f_to_str(ObjectTrailsColor[1]));
	conf.set_str    ("colorc:chart_color", StelUtility::vec3f_to_str(ChartColor[1]));

	conf.set_str    ("colorr:azimuthal_color", StelUtility::vec3f_to_str(AzimuthalColor[2]));
	conf.set_str    ("colorr:equatorial_color", StelUtility::vec3f_to_str(EquatorialColor[2]));
	conf.set_str    ("colorr:equator_color", StelUtility::vec3f_to_str(EquatorColor[2]));
	conf.set_str    ("colorr:ecliptic_color", StelUtility::vec3f_to_str(EclipticColor[2]));
	conf.set_str    ("colorr:const_lines_color", StelUtility::vec3f_to_str(ConstLinesColor[2]));
	conf.set_str    ("colorr:const_names_color", StelUtility::vec3f_to_str(ConstNamesColor[2]));
	conf.set_str    ("colorr:const_boundary_color", StelUtility::vec3f_to_str(ConstBoundaryColor[2]));
	conf.set_str	("colorr:nebula_label_color", StelUtility::vec3f_to_str(NebulaLabelColor[2]));
	conf.set_str	("colorr:nebula_circle_color", StelUtility::vec3f_to_str(NebulaCircleColor[2]));
	conf.set_str	("colorr:star_label_color", StelUtility::vec3f_to_str(StarLabelColor[2]));
	conf.set_str	("colorr:star_circle_color", StelUtility::vec3f_to_str(StarCircleColor[2]));
	conf.set_str    ("colorr:cardinal_color", StelUtility::vec3f_to_str(CardinalColor[2]));
	conf.set_str    ("colorr:planet_names_color", StelUtility::vec3f_to_str(PlanetNamesColor[2]));
	conf.set_str    ("colorr:planet_orbits_color", StelUtility::vec3f_to_str(PlanetOrbitsColor[2]));
	conf.set_str    ("colorr:object_trails_color", StelUtility::vec3f_to_str(ObjectTrailsColor[2]));
	conf.set_str    ("colorr:chart_color", StelUtility::vec3f_to_str(ChartColor[2]));

	// Text ui section
	conf.set_boolean("tui:flag_enable_tui_menu", FlagEnableTuiMenu);
	conf.set_boolean("tui:flag_show_gravity_ui", FlagShowGravityUi);
	conf.set_boolean("tui:flag_show_tui_datetime", FlagShowTuiDateTime);
	conf.set_boolean("tui:flag_show_tui_short_obj_info", FlagShowTuiShortObjInfo);

	// Navigation section
	conf.set_double ("navigation:preset_sky_time", PresetSkyTime);
	conf.set_str	("navigation:startup_time_mode", StartupTimeMode);
	conf.set_boolean("navigation:flag_enable_zoom_keys", FlagEnableZoomKeys);
	conf.set_boolean("navigation:flag_manual_zoom", FlagManualZoom);
	conf.set_boolean("navigation:flag_enable_move_keys", FlagEnableMoveKeys);
	conf.set_boolean("navigation:flag_enable_move_mouse", FlagEnableMoveMouse);
	conf.set_double ("navigation:init_fov", InitFov);
	conf.set_str	("navigation:init_view_pos", StelUtility::vec3f_to_str(InitViewPos));
	conf.set_double ("navigation:auto_move_duration", auto_move_duration);
	conf.set_boolean("navigation:flag_utc_time", FlagUTC_Time);
	conf.set_int    ("navigation:mouse_zoom", MouseZoom);
	conf.set_double ("navigation:move_speed", move_speed);
	conf.set_double ("navigation:zoom_speed", zoom_speed);

	switch (navigation->get_viewing_mode())
	{
	case Navigator::VIEW_HORIZON : tmpstr="horizon";	break;
	case Navigator::VIEW_EQUATOR : tmpstr="equator";		break;
	default : tmpstr="horizon";
	}
	conf.set_str	("navigation:viewing_mode",tmpstr);

	// Landscape section
	conf.set_boolean("landscape:flag_landscape", getFlagLandscape());
	conf.set_boolean("landscape:flag_fog", getFlagFog());
	conf.set_boolean("landscape:flag_atmosphere", getFlagAtmosphere());
	conf.set_double ("viewing:atmosphere_fade_duration", getAtmosphereFadeDuration());

	// Viewing section
	conf.set_boolean("viewing:flag_constellation_drawing", getFlagConstellationLines());
	conf.set_boolean("viewing:flag_constellation_name", getFlagConstellationNames());
	conf.set_boolean("viewing:flag_constellation_art", getFlagConstellationArt());
	conf.set_boolean("viewing:flag_constellation_boundaries", getFlagConstellationBoundaries());
	conf.set_boolean("viewing:flag_constellation_isolate_selected", getFlagConstellationIsolateSelected());

	conf.set_boolean("viewing:flag_azimutal_grid", getFlagAzimutalGrid());
	conf.set_boolean("viewing:flag_equatorial_grid", getFlagEquatorGrid());
	conf.set_boolean("viewing:flag_equator_line", getFlagEquatorLine());
	conf.set_boolean("viewing:flag_ecliptic_line", getFlagEclipticLine());
	conf.set_boolean("viewing:flag_meridian_line", getFlagMeridianLine());
	conf.set_boolean("viewing:flag_cardinal_points", cardinals_points->getFlagShow());
	conf.set_boolean("viewing:flag_gravity_labels", projection->getFlagGravityLabels());
	conf.set_boolean("viewing:flag_moon_scaled", getFlagMoonScaled());
	conf.set_double ("viewing:moon_scale", getMoonScale());
	conf.set_double ("viewing:constellation_art_intensity", getConstellationArtIntensity());
	conf.set_double ("viewing:constellation_art_fade_duration", getConstellationArtFadeDuration());
	conf.set_boolean("viewing:flag_chart", FlagChart);
	conf.set_boolean("viewing:flag_night", FlagNight);

	// Astro section
	conf.set_boolean("astro:flag_stars", getFlagStars());
	conf.set_boolean("astro:flag_star_name", getFlagStarName());
	conf.set_boolean("astro:flag_planets", getFlagPlanets());
	conf.set_boolean("astro:flag_planets_hints", getFlagPlanetsHints());
	conf.set_boolean("astro:flag_planets_orbits", getFlagPlanetsOrbits());
	conf.set_boolean("astro:flag_object_trails", getFlagPlanetsTrails());
	conf.set_boolean("astro:flag_nebula", getFlagNebula());
	conf.set_boolean("astro:flag_nebula_name", getFlagNebulaHints());
	conf.set_double("astro:max_mag_nebula_name", getNebulaMaxMagHints());
	conf.set_double("astro:nebula_scale", getNebulaCircleScale());
	conf.set_boolean("astro:flag_milky_way", getFlagMilkyWay());
	conf.set_double("astro:milky_way_intensity", getMilkyWayIntensity());
	conf.set_boolean("astro:flag_bright_nebulae", getFlagNebulaBright());

	conf.save(confFile);
}


// Handle mouse clics
int StelCore::handleClick(Uint16 x, Uint16 y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	return ui->handle_clic(x, y, button, state);
}

// Handle mouse move
int StelCore::handleMove(int x, int y)
{
	// Turn if the mouse is at the edge of the screen.
	// unless config asks otherwise
	if(FlagEnableMoveMouse)
	{
		if (x == 0)
		{
			turn_left(1);
			is_mouse_moving_horiz = true;
		}
		else if (x == screen_W - 1)
		{
			turn_right(1);
			is_mouse_moving_horiz = true;
		}
		else if (is_mouse_moving_horiz)
		{
			turn_left(0);
			is_mouse_moving_horiz = false;
		}

		if (y == 0)
		{
			turn_up(1);
			is_mouse_moving_vert = true;
		}
		else if (y == screen_H - 1)
		{
			turn_down(1);
			is_mouse_moving_vert = true;
		}
		else if (is_mouse_moving_vert)
		{
			turn_up(0);
			is_mouse_moving_vert = false;
		}
	}

	return ui->handle_move(x, y);

}

// Handle key press and release
int StelCore::handleKeys(Uint16 key, s_gui::S_GUI_VALUE state)
{
	s_tui::S_TUI_VALUE tuiv;
	if (state == s_gui::S_GUI_PRESSED) tuiv = s_tui::S_TUI_PRESSED;
	else tuiv = s_tui::S_TUI_RELEASED;
	if (FlagShowTuiMenu)
	{

		if (state==S_GUI_PRESSED && key==SDLK_m)
		{
			// leave tui menu
			FlagShowTuiMenu = false;

			// If selected a script in tui, run that now
			if(SelectedScript!="")
				commander->execute_command("script action play filename " +  SelectedScript
				                           + " path " + SelectedScriptDirectory);

			// clear out now
			SelectedScriptDirectory = SelectedScript = "";
			return 1;
		}
		if (ui->handle_keys_tui(key, tuiv)) return 1;
		return 1;
	}

	if (ui->handle_keys(key, state)) return 1;

	if (state == S_GUI_PRESSED)
	{
		// Direction and zoom deplacements
		if (key==SDLK_LEFT) turn_left(1);
		if (key==SDLK_RIGHT) turn_right(1);
		if (key==SDLK_UP)
		{
			if (SDL_GetModState() & KMOD_CTRL) zoom_in(1);
			else turn_up(1);
		}
		if (key==SDLK_DOWN)
		{
			if (SDL_GetModState() & KMOD_CTRL) zoom_out(1);
			else turn_down(1);
		}
		if (key==SDLK_PAGEUP) zoom_in(1);
		if (key==SDLK_PAGEDOWN) zoom_out(1);
	}
	else
	{
		// When a deplacement key is released stop mooving
		if (key==SDLK_LEFT) turn_left(0);
		if (key==SDLK_RIGHT) turn_right(0);
		if (SDL_GetModState() & KMOD_CTRL)
		{
			if (key==SDLK_UP) zoom_in(0);
			if (key==SDLK_DOWN) zoom_out(0);
		}
		else
		{
			if (key==SDLK_UP) turn_up(0);
			if (key==SDLK_DOWN) turn_down(0);
		}
		if (key==SDLK_PAGEUP) zoom_in(0);
		if (key==SDLK_PAGEDOWN) zoom_out(0);
	}
	return 0;
}

void StelCore::turn_right(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = 1;
		navigation->set_flag_traking(0);
		navigation->set_flag_lock_equ_pos(0);
	}
	else deltaAz = 0;
}

void StelCore::turn_left(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = -1;
		navigation->set_flag_traking(0);
		navigation->set_flag_lock_equ_pos(0);

	}
	else deltaAz = 0;
}

void StelCore::turn_up(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = 1;
		navigation->set_flag_traking(0);
		navigation->set_flag_lock_equ_pos(0);
	}
	else deltaAlt = 0;
}

void StelCore::turn_down(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = -1;
		navigation->set_flag_traking(0);
		navigation->set_flag_lock_equ_pos(0);
	}
	else deltaAlt = 0;
}

void StelCore::zoom_in(int s)
{
	if (FlagEnableZoomKeys) deltaFov = -1*(s!=0);
}

void StelCore::zoom_out(int s)
{
	if (FlagEnableZoomKeys) deltaFov = (s!=0);
}

// Increment/decrement smoothly the vision field and position
void StelCore::updateMove(int delta_time)
{
	// the more it is zoomed, the more the mooving speed is low (in angle)
	double depl=move_speed*delta_time*projection->get_fov();
	double deplzoom=zoom_speed*delta_time*projection->get_fov();
	if (deltaAz<0)
	{
		deltaAz = -depl/30;
		if (deltaAz<-0.2) deltaAz = -0.2;
	}
	else
	{
		if (deltaAz>0)
		{
			deltaAz = (depl/30);
			if (deltaAz>0.2) deltaAz = 0.2;
		}
	}
	if (deltaAlt<0)
	{
		deltaAlt = -depl/30;
		if (deltaAlt<-0.2) deltaAlt = -0.2;
	}
	else
	{
		if (deltaAlt>0)
		{
			deltaAlt = depl/30;
			if (deltaAlt>0.2) deltaAlt = 0.2;
		}
	}

	if (deltaFov<0)
	{
		deltaFov = -deplzoom*5;
		if (deltaFov<-0.15*projection->get_fov()) deltaFov = -0.15*projection->get_fov();
	}
	else
	{
		if (deltaFov>0)
		{
			deltaFov = deplzoom*5;
			if (deltaFov>20) deltaFov = 20;
		}
	}

	//	projection->change_fov(deltaFov);
	//	navigation->update_move(deltaAz, deltaAlt);

	if(deltaFov != 0 )
	{
		std::ostringstream oss;
		oss << "zoom delta_fov " << deltaFov;
		commander->execute_command(oss.str());
	}

	if(deltaAz != 0 || deltaAlt != 0)
	{
		std::ostringstream oss;
		oss << "look delta_az " << deltaAz << " delta_alt " << deltaAlt;
		commander->execute_command(oss.str());
	}
	else
	{
		// must perform call anyway, but don't record!
		navigation->update_move(deltaAz, deltaAlt);
	}
}
