/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�reau
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

#include "stel_ui.h"

using namespace s_gui;


Component* stel_ui::createConfigWindow(void)
{
	config_win = new StdBtWin("Configuration");
	config_win->reshape(300,200,400,350);
	config_win->setVisible(core->FlagConfig);

	config_tab_ctr = new TabContainer();
	config_tab_ctr->setSize(config_win->getSize());

	// The current drawing position
	int x,y;
	x=70; y=15;

	// Rendering options
	FilledContainer* tab_render = new FilledContainer();
	tab_render->setSize(config_tab_ctr->getSize());

	s_texture* starp = new s_texture("halo");
	Picture* pstar = new Picture(starp, x-50, y+5, 32, 32);
	tab_render->addComponent(pstar);

	stars_cbx = new LabeledCheckBox(core->FlagStars, "Stars");
	stars_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(stars_cbx);
	stars_cbx->setPos(x,y); y+=15;

	star_names_cbx = new LabeledCheckBox(core->FlagStarName, "Star Names. Up to mag :");
	star_names_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_names_cbx);
	star_names_cbx->setPos(x,y);

	max_mag_star_name = new FloatIncDec(courierFont, tex_up, tex_down, -1.5, 9,
		core->MaxMagStarName, 0.5);
	max_mag_star_name->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(max_mag_star_name);
	max_mag_star_name->setPos(x + 220,y);

	y+=15;

	star_twinkle_cbx = new LabeledCheckBox(core->FlagStarTwinkle, "Star Twinkle. Amount :");
	star_twinkle_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_twinkle_cbx);
	star_twinkle_cbx->setPos(x,y);

	star_twinkle_amount = new FloatIncDec(courierFont, tex_up, tex_down, 0, 0.6,
		core->StarTwinkleAmount, 0.1);
	star_twinkle_amount->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_twinkle_amount);
	star_twinkle_amount->setPos(x + 220,y);

	y+=30;

	s_texture* constellp = new s_texture("bt_constellations");
	Picture* pconstell = new Picture(constellp, x-50, y+5, 32, 32);
	tab_render->addComponent(pconstell);

	constellation_cbx = new LabeledCheckBox(core->FlagConstellationDrawing, "Constellations");
	constellation_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(constellation_cbx);
	constellation_cbx->setPos(x,y); y+=15;

	constellation_name_cbx = new LabeledCheckBox(core->FlagConstellationName, "Constellations Names");
	constellation_name_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(constellation_name_cbx);
	constellation_name_cbx->setPos(x,y); y+=15;

	sel_constellation_cbx = new LabeledCheckBox(core->FlagConstellationPick, "Selected Constellation Only");
	sel_constellation_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(sel_constellation_cbx);
	sel_constellation_cbx->setPos(x,y);

	y+=25;

	s_texture* nebp = new s_texture("bt_nebula");
	Picture* pneb = new Picture(nebp, x-50, y, 32, 32);
	tab_render->addComponent(pneb);

	nebulas_cbx = new LabeledCheckBox(core->FlagNebula, "Nebulas");
	nebulas_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(nebulas_cbx);
	nebulas_cbx->setPos(x,y); y+=15;

	nebulas_names_cbx = new LabeledCheckBox(core->FlagNebulaName, "Nebulas Names. Up to mag :");
	nebulas_names_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(nebulas_names_cbx);
	nebulas_names_cbx->setPos(x,y);

	max_mag_nebula_name = new FloatIncDec(courierFont, tex_up, tex_down, 0, 12,
		core->MaxMagNebulaName, 0.5);
	max_mag_nebula_name->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(max_mag_nebula_name);
	max_mag_nebula_name->setPos(x + 220,y);

	y+=30;

	s_texture* planp = new s_texture("bt_planet");
	Picture* pplan = new Picture(planp, x-50, y, 32, 32);
	tab_render->addComponent(pplan);

	planets_cbx = new LabeledCheckBox(core->FlagPlanets, "Planets");
	planets_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(planets_cbx);
	planets_cbx->setPos(x,y);

	moon_x4_cbx = new LabeledCheckBox(core->ssystem->get_moon()->get_sphere_scale()!=1.f, "Moon Scale");
	moon_x4_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(moon_x4_cbx);
	moon_x4_cbx->setPos(x + 150,y);

	y+=15;

	planets_hints_cbx = new LabeledCheckBox(core->FlagPlanetsHints, "Planets Hints");
	planets_hints_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(planets_hints_cbx);
	planets_hints_cbx->setPos(x,y);

	y+=25;

	s_texture* gridp = new s_texture("bt_grid");
	Picture* pgrid = new Picture(gridp, x-50, y, 32, 32);
	tab_render->addComponent(pgrid);

	equator_grid_cbx = new LabeledCheckBox(core->FlagEquatorialGrid, "Equatorial Grid");
	equator_grid_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(equator_grid_cbx);
	equator_grid_cbx->setPos(x,y); y+=15;

	azimuth_grid_cbx = new LabeledCheckBox(core->FlagAzimutalGrid, "Azimuthal Grid");
	azimuth_grid_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(azimuth_grid_cbx);
	azimuth_grid_cbx->setPos(x,y); y-=15;

	equator_cbx = new LabeledCheckBox(core->FlagEquatorLine, "Equator Line");
	equator_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(equator_cbx);
	equator_cbx->setPos(x + 150,y); y+=15;

	ecliptic_cbx = new LabeledCheckBox(core->FlagEclipticLine, "Ecliptic Line");
	ecliptic_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(ecliptic_cbx);
	ecliptic_cbx->setPos(x + 150,y);

	y+=25;

	s_texture* groundp = new s_texture("bt_ground");
	Picture* pground = new Picture(groundp, x-50, y, 32, 32);
	tab_render->addComponent(pground);

	ground_cbx = new LabeledCheckBox(core->FlagGround, "Ground ");
	ground_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(ground_cbx);
	ground_cbx->setPos(x,y);

	cardinal_cbx = new LabeledCheckBox(core->FlagCardinalPoints, "Cardinal Points");
	cardinal_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(cardinal_cbx);
	cardinal_cbx->setPos(x + 150,y); y+=15;

	atmosphere_cbx = new LabeledCheckBox(core->FlagAtmosphere, "Atmosphere");
	atmosphere_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(atmosphere_cbx);
	atmosphere_cbx->setPos(x,y);

	fog_cbx = new LabeledCheckBox(core->FlagFog, "Fog");
	fog_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(fog_cbx);
	fog_cbx->setPos(x + 150,y); y+=30;

	LabeledButton* render_save_bt = new LabeledButton("Save as default");
	render_save_bt->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(render_save_bt);
	render_save_bt->setPos(x + 80,y); y+=20;

/*	CursorBar* star_scale_cbar = new CursorBar(0,20,2);
	star_scale_cbar->setOnChangeCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_scale_cbar);
	star_scale_cbar->setPos(x,y); y+=15;*/

	// Date & Time options
	FilledContainer* tab_time = new FilledContainer();
	tab_time->setSize(config_tab_ctr->getSize());

	x=10; y=10;

	Label* tclbl = new Label("\1 Current Time :");
	tclbl->setPos(x,y); y+=20;
	tab_time->addComponent(tclbl);

	time_current = new Time_item(courierFont, tex_up, tex_down);
	time_current->setOnChangeTimeCallback(callback<void>(this, &stel_ui::setCurrentTimeFromConfig));
	tab_time->addComponent(time_current);
	time_current->setPos(50,y); y+=80;

	Label* tzbl = new Label("\1 Time Zone :");
	tzbl->setPos(x,y); y+=20;
	tab_time->addComponent(tzbl);

	system_tz_cbx = new LabeledCheckBox(core->observatory->get_tz_format()==S_TZ_SYSTEM_DEFAULT,
		"Use System Default Time Zone");
	system_tz_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_time->addComponent(system_tz_cbx);
	system_tz_cbx->setPos(50 ,y); y+=30;

	tzselector = new Time_zone_item(core->DataDir + "zone.tab");
	tzselector->setOnPressCallback(callback<void>(this, &stel_ui::setTimeZone));
	tzselector->setPos(x,y);
	tab_time->addComponent(tzselector);


	// Location options
	FilledContainer* tab_location = new FilledContainer();
	tab_location->setSize(config_tab_ctr->getSize());

	x=5; y=5;
	s_texture * earth = new s_texture("earthmap");
	s_texture * pointertex = new s_texture("neb");
	earth_map = new MapPicture(earth, pointertex, x, y, tab_location->getSizex()-10, 250);
	earth_map->setOnPressCallback(callback<void>(this, &stel_ui::setObserverPositionFromMap));
	tab_location->addComponent(earth_map);
	y+=earth_map->getSizey() + 20;

	Label * lbllong = new Label("Longitude : ");
	lbllong->setPos(30, y+1);
	Label * lbllat = new Label("Latitude : ");
	lbllat->setPos(30, y+21);
	tab_location->addComponent(lbllong);
	tab_location->addComponent(lbllat);

	long_incdec	= new FloatIncDec(courierFont, tex_up, tex_down, -180, 180, 0, 0.05);
	long_incdec->setSizex(100);
	long_incdec->setOnPressCallback(callback<void>(this, &stel_ui::setObserverPositionFromIncDec));
	long_incdec->setPos(110,y);
	lat_incdec	= new FloatIncDec(courierFont, tex_up, tex_down, -90, 90, 0, 0.05);
	lat_incdec->setSizex(100);
	lat_incdec->setOnPressCallback(callback<void>(this, &stel_ui::setObserverPositionFromIncDec));
	lat_incdec->setPos(110,y+20);
	tab_location->addComponent(long_incdec);
	tab_location->addComponent(lat_incdec);

	LabeledButton* location_save_bt = new LabeledButton("Save location");
	location_save_bt->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	location_save_bt->setPos(200,y+5);
	tab_location->addComponent(location_save_bt);

	// Video Options
	FilledContainer* tab_video = new FilledContainer();
	tab_video->setSize(config_tab_ctr->getSize());

	x=5; y=5;


	config_tab_ctr->setTexture(flipBaseTex);
	config_tab_ctr->addTab(tab_time, "Date & Time");
	config_tab_ctr->addTab(tab_location, "Location");
	config_tab_ctr->addTab(tab_render, "Rendering");
	config_tab_ctr->addTab(tab_video, "Video");
	config_win->addComponent(config_tab_ctr);
	config_win->setOnHideBtCallback(callback<void>(this, &stel_ui::config_win_hideBtCallback));
	return config_win;
}

void stel_ui::updateConfigVariables(void)
{
	core->FlagStars = stars_cbx->getState();
	core->FlagStarName = star_names_cbx->getState();
	core->MaxMagStarName = max_mag_star_name->getValue();
	core->FlagStarTwinkle = star_twinkle_cbx->getState();
	core->StarTwinkleAmount = star_twinkle_amount->getValue();
	core->FlagConstellationDrawing = constellation_cbx->getState();
	core->FlagConstellationName = constellation_name_cbx->getState();
	core->FlagConstellationPick = sel_constellation_cbx->getState();
	core->FlagNebula = nebulas_cbx->getState();
	core->FlagNebulaName = nebulas_names_cbx->getState();
	core->MaxMagNebulaName = max_mag_nebula_name->getValue();
	core->FlagPlanets = planets_cbx->getState();
	core->FlagPlanetsHints = planets_hints_cbx->getState();
	core->ssystem->get_moon()->set_sphere_scale(moon_x4_cbx->getState() ? core->moon_scale : 1.f);
	core->FlagEquatorialGrid = equator_grid_cbx->getState();
	core->FlagAzimutalGrid = azimuth_grid_cbx->getState();
	core->FlagEquatorLine = equator_cbx->getState();
	core->FlagEclipticLine = ecliptic_cbx->getState();
	core->FlagGround = ground_cbx->getState();
	core->FlagCardinalPoints = cardinal_cbx->getState();
	core->FlagAtmosphere = atmosphere_cbx->getState();
	core->FlagFog = fog_cbx->getState();
}

void stel_ui::setCurrentTimeFromConfig(void)
{
	core->navigation->set_JDay(time_current->getJDay() - core->observatory->get_GMT_shift()*JD_HOUR);
}

void stel_ui::setObserverPositionFromMap(void)
{
	core->observatory->set_latitude(earth_map->getPointerLatitude());
	core->observatory->set_longitude(earth_map->getPointerLongitude());
}

void stel_ui::setObserverPositionFromIncDec(void)
{
	core->observatory->set_latitude(lat_incdec->getValue());
	core->observatory->set_longitude(long_incdec->getValue());
}

void stel_ui::setTimeZone(void)
{
	core->observatory->set_custom_tz_name(tzselector->gettz());
}

void stel_ui::updateConfigForm(void)
{
	stars_cbx->setState(core->FlagStars);
	star_names_cbx->setState(core->FlagStarName);
	max_mag_star_name->setValue(core->MaxMagStarName);
	star_twinkle_cbx->setState(core->FlagStarTwinkle);
	star_twinkle_amount->setValue(core->StarTwinkleAmount);
	constellation_cbx->setState(core->FlagConstellationDrawing);
	constellation_name_cbx->setState(core->FlagConstellationName);
	sel_constellation_cbx->setState(core->FlagConstellationPick);
	nebulas_cbx->setState(core->FlagNebula);
	nebulas_names_cbx->setState(core->FlagNebulaName);
	max_mag_nebula_name->setValue(core->MaxMagNebulaName);
	planets_cbx->setState(core->FlagPlanets);
	planets_hints_cbx->setState(core->FlagPlanetsHints);
	moon_x4_cbx->setState(core->ssystem->get_moon()->get_sphere_scale()!=1.f);
	equator_grid_cbx->setState(core->FlagEquatorialGrid);
	azimuth_grid_cbx->setState(core->FlagAzimutalGrid);
	equator_cbx->setState(core->FlagEquatorLine);
	ecliptic_cbx->setState(core->FlagEclipticLine);
	ground_cbx->setState(core->FlagGround);
	cardinal_cbx->setState(core->FlagCardinalPoints);
	atmosphere_cbx->setState(core->FlagAtmosphere);
	fog_cbx->setState(core->FlagFog);

	earth_map->setPointerLongitude(core->observatory->get_longitude());
	earth_map->setPointerLatitude(core->observatory->get_latitude());
	long_incdec->setValue(core->observatory->get_longitude());
	lat_incdec->setValue(core->observatory->get_latitude());

	time_current->setJDay(core->navigation->get_JDay() + core->observatory->get_GMT_shift()*JD_HOUR);
}

void stel_ui::config_win_hideBtCallback(void)
{
	core->FlagConfig = false;
	config_win->setVisible(false);
	bt_flag_config->setState(0);
}
