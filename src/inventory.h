/* *
 *
 * File:  inventory.h
 * Brief: Items carried by a character
 * 
 * Copyright (C) 2016-2017 Matyas Cerny <cerny-matyas(at)email.cz>
 *
 *
 * This file is part of Dun-geon.
 *
 * Dun-geon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dun-geon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dun-geon.  If not, see <http://www.gnu.org/licenses/>.
 *
 * */

#ifndef _INVENTORY_H
#define _INVENTORY_H

#include <vector>

class Item;

class Inventory {
private:
	std::vector<Item *> items;
public:
	std::vector<Item *> getItems();
	Item *getItem(int index);
	void addItem(Item *item);
	void removeItem(int index);
	void sortItemsByName();
	void sortItemsByType();
	void sortItemsByCount();
};

#endif
