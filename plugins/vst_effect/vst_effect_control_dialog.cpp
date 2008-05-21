/*
 * vst_effect_control_dialog.cpp - dialog for displaying VST-effect GUI
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QtGui/QLayout>
#include <QtGui/QMdiArea>

#include "vst_effect_control_dialog.h"
#include "vst_effect.h"
#include "main_window.h"



vstEffectControlDialog::vstEffectControlDialog( vstEffectControls * _ctl ) :
	effectControlDialog( _ctl )
{
	QVBoxLayout * l = new QVBoxLayout( this );
	QWidget * w = _ctl->m_effect->m_plugin->showEditor( this );
	if( w )
	{
		w->show();
		l->addWidget( w );
	}
}




vstEffectControlDialog::~vstEffectControlDialog()
{
}

