/*
 * spectrumanaylzer_control_dialog.cpp - view for spectrum analyzer
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QPainter>

#include "spectrum_analyzer.h"
#include "MainWindow.h"
#include "led_checkbox.h"
#include "embed.h"


static inline void darken( QImage & _i, int _x, int _y, int _w, int _h )
{
	const int w = _i.width();
	QRgb * base = ( (QRgb *) _i.bits() ) + _y*w + _x;
	for( int y = 0; y < _h; ++y )
	{
		QRgb * d = base + y*w;
		for( int x = 0; x < _w; ++x )
		{
			// shift each color component by 1 bit and set alpha
			// to 0xff
			d[x] = ( ( d[x] >> 1 ) & 0x7f7f7f7f ) | 0xff000000;
		}
	}
}



class spectrumView : public QWidget
{
public:
	spectrumView( spectrumAnalyzer * _s, QWidget * _parent ) :
		QWidget( _parent ),
		m_sa( _s ),
		m_backgroundPlain( PLUGIN_NAME::getIconPixmap( "spectrum_background_plain" ).toImage() ),
		m_background( PLUGIN_NAME::getIconPixmap( "spectrum_background" ).toImage() )
	{
		setFixedSize( 249, 151 );
		connect( engine::mainWindow(), SIGNAL( periodicUpdate() ),
				this, SLOT( update() ) );
		setAttribute( Qt::WA_OpaquePaintEvent, true );
	}

	virtual ~spectrumView()
	{
	}

	virtual void paintEvent( QPaintEvent * _pe )
	{
		QPainter p( this );
		QImage i = m_sa->m_saControls.m_linearSpec.value() ?
					m_backgroundPlain : m_background;
		const float e = m_sa->m_energy;
		if( e <= 0 )
		{
			darken( i, 0, 0, i.width(), i.height() );
			p.drawImage( 0, 0, i );
			return;
		}

		const bool lin_y = m_sa->m_saControls.m_linearYAxis.value();
		float * b = m_sa->m_bands;
		const int LOWER_Y = -60;	// dB
		int h;
		const int fh = height();
		if( m_sa->m_saControls.m_linearSpec.value() )
		{
			if( lin_y )
			{
				for( int x = 0; x < MAX_BANDS; ++x, ++b )
				{
					h = fh * 2.0 / 3.0 * (*b / e );
					if( h < 0 ) h = 0; else if( h >= fh ) continue;
					darken( i, x, 0, 1, fh-h );
				}
			}
			else
			{
				for( int x = 0; x < MAX_BANDS; ++x, ++b )
				{
					h = (int)( fh * 2.0 / 3.0 * (20*(log10( *b / e ) ) - LOWER_Y ) / (-LOWER_Y ) );
					if( h < 0 ) h = 0; else if( h >= fh ) continue;
					darken( i, x, 0, 1, fh-h );
				}
			}
		}
		else
		{
			if( lin_y )
			{
				for( int x = 0; x < 31; ++x, ++b )
				{
					h = fh * 2.0 / 3.0 * ( 1.2 * *b / e );
					if( h < 0 ) h = 0; else if( h >= fh ) continue; else h = ( h / 3 ) * 3;
					darken( i, x*8, 0, 8, fh-h );
				}
			}
			else
			{
				for( int x = 0; x < 31; ++x, ++b )
				{
					h = (int)( fh * 2.0 / 3.0 * (20*(log10( *b / e ) ) - LOWER_Y ) / (-LOWER_Y ) );
					if( h < 0 ) h = 0; else if( h >= fh ) continue; else h = ( h / 3 ) * 3;
					darken( i, x*8, 0, 8, fh-h );
				}
			}
			darken( i, 31*8, 0, 1, fh );
		}
		p.drawImage( 0, 0, i );
	}


private:
	spectrumAnalyzer * m_sa;
	QImage m_backgroundPlain;
	QImage m_background;

} ;




spectrumAnalyzerControlDialog::spectrumAnalyzerControlDialog(
					spectrumAnalyzerControls * _controls ) :
	EffectControlDialog( _controls ),
	m_controls( _controls ),
	m_logXAxis( PLUGIN_NAME::getIconPixmap( "log_x_axis" ) ),
	m_logYAxis( PLUGIN_NAME::getIconPixmap( "log_y_axis" ) )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "background" ) );
	setFixedSize( 280, 243 );
	setPalette( pal );
/*	QVBoxLayout * l = new QVBoxLayout( this );*/
	spectrumView * v = new spectrumView( _controls->m_effect, this );
	v->move( 27, 30 );

	ledCheckBox * lin_spec = new ledCheckBox( tr( "Linear spectrum" ), this );
	lin_spec->move( 24, 204 );
	lin_spec->setModel( &_controls->m_linearSpec );

	ledCheckBox * lin_y = new ledCheckBox( tr( "Linear Y axis" ), this );
	lin_y->move( 24, 220 );
	lin_y->setModel( &_controls->m_linearYAxis );

	connect( &_controls->m_linearSpec, SIGNAL( dataChanged() ),
			this, SLOT( update() ) );
	connect( &_controls->m_linearYAxis, SIGNAL( dataChanged() ),
			this, SLOT( update() ) );
/*	l->addWidget( v );
	l->addWidget( lin_spec );
	l->addWidget( lin_y );*/

}


void spectrumAnalyzerControlDialog::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	if( !m_controls->m_linearSpec.value() )
	{
		p.drawPixmap( 25, 183, m_logXAxis );
	}
	if( !m_controls->m_linearYAxis.value() )
	{
		p.drawPixmap( 3, 47, m_logYAxis);
	}

}



