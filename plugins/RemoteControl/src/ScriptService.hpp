/*
 * Stellarium Remote Control plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef SCRIPTSERVICE_HPP_
#define SCRIPTSERVICE_HPP_

#include "APIController.hpp"

class StelScriptMgr;

class ScriptService : public AbstractAPIService
{
	Q_OBJECT
public:
	ScriptService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~ScriptService() {}

	virtual void get(const QList<QByteArray>& args,const QMultiMap<QByteArray,QByteArray>& parameters, HttpResponse& response);
private:
	StelScriptMgr* scriptMgr;
};



#endif
