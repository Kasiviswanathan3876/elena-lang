//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA IDE
//      WindowList class implementation
//                                              (C)2005-2011, by Alexei Rakov
//---------------------------------------------------------------------------

#include "windowlist.h"

using namespace _GUI_;
using namespace _ELENA_;

// --- WindowList

WindowList :: WindowList(int maxCount, int menuBaseId)
   : MenuHistoryList(maxCount, menuBaseId, false)
{
}

void WindowList :: add(const tchar_t* item)
{
   if (emptystr(item))
      return;

#ifdef _WIN32
   for (size_t i = 0 ; i < _menuSize ; i++) {
      _menu->checkItemById(_menuBaseId + i + 1, false);
   }

   int index = getIndex(item);
   if (index == -1) {
      MenuHistoryList::add(item);

      _menu->checkItemById(_menuBaseId + 1, true);
   }
   else _menu->checkItemById(_menuBaseId + index + 1, true);
#endif
}

void WindowList :: remove(const tchar_t* item)
{
#ifdef _WIN32
   if (erase(item))
      refresh();
#endif
}
