#include "pch.h"
// 심볼 링크용
extern void LinkEngineComponents();

static bool ForceLink = []() {
    LinkEngineComponents();
    return true;
    }();