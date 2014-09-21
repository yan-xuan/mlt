/**
 * MltField.h - Field wrapper
 * Copyright (C) 2004-2005 Charles Yates
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MLTPP_FIELD_H
#define MLTPP_FIELD_H

#include "config.h"

#include <framework/mlt.h>

#include "MltService.h"

namespace Mlt
{
	class Service;
	class Filter;
	class Transition;

	class MLTPP_DECLSPEC Field : public Service
	{
		private:
			mlt_field instance;
		public:
			Field( mlt_field field );
			Field( Field &field );
			virtual ~Field( );
			mlt_field get_field( );
			mlt_service get_service( );
			int plant_filter( Filter &filter, int track = 0 );
			int plant_transition( Transition &transition, int a_track = 0, int b_track = 1 );
			void disconnect_service( Service &service );
	};
}

#endif

