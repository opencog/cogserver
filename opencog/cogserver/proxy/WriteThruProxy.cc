/*
 * opencog/cogserver/proxy/WriteThruProxy.cc
 *
 * Simple WriteThru shell
 * Copyright (c) 2008, 2020 Linas Vepstas <linas@linas.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cstdio>
#include "WriteThruProxy.h"

using namespace opencog;

DECLARE_MODULE(WriteThruProxy);

WriteThruProxy::WriteThruProxy(CogServer& cs) : Proxy(cs)
{
printf("duuuude proxy ctor\n");
}

void WriteThruProxy::init(void)
{
printf("duuuude proxy init\n");
}

WriteThruProxy::~WriteThruProxy()
{
}

bool WriteThruProxy::config(const char* cfg)
{
printf("duuuude proxy cfg %s\n", cfg);
	return false;
}

void WriteThruProxy::stuff(void)
{
printf("duuuude proxy install stufffff!!\n");
}

/* ===================== END OF FILE ============================ */
