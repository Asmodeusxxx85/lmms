/*
 * volume_knob.h - defines a knob that display it's value as either a
 *                 percentage or in dbV.
 *
 * Copyright (c) 2006  Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
#ifndef _VOLUME_KNOB_H
#define _VOLUME_KNOB_H

#ifdef QT4

#include <QtGui/QPixmap>

#else

#include <qpixmap.h>

#endif

#include "types.h"
#include "knob.h"


class volumeKnob: public knob
{
	Q_OBJECT
public:
	volumeKnob( int _knob_num, QWidget * _parent, const QString & _name,
		    engine * _engine, const int _knob_id = 0 );
	~volumeKnob();

public slots:
	void enterValue( void );

protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void wheelEvent( QWheelEvent * _we );
};

#endif
