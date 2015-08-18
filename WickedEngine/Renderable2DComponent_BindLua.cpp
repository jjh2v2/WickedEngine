#include "Renderable2DComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"
#include "wiSprite_BindLua.h"

const char Renderable2DComponent_BindLua::className[] = "Renderable2DComponent";

Luna<Renderable2DComponent_BindLua>::FunctionType Renderable2DComponent_BindLua::methods[] = {
	lunamethod(Renderable2DComponent_BindLua, GetContent),
	lunamethod(Renderable2DComponent_BindLua, AddSprite),
	lunamethod(Renderable2DComponent_BindLua, Initialize),
	lunamethod(Renderable2DComponent_BindLua, Load),
	lunamethod(Renderable2DComponent_BindLua, Unload),
	lunamethod(Renderable2DComponent_BindLua, Start),
	lunamethod(Renderable2DComponent_BindLua, Stop),
	lunamethod(Renderable2DComponent_BindLua, Update),
	lunamethod(Renderable2DComponent_BindLua, Render),
	lunamethod(Renderable2DComponent_BindLua, Compose),
	{ NULL, NULL }
};
Luna<Renderable2DComponent_BindLua>::PropertyType Renderable2DComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

Renderable2DComponent_BindLua::Renderable2DComponent_BindLua(Renderable2DComponent* component)
{
	this->component = component;
}

Renderable2DComponent_BindLua::Renderable2DComponent_BindLua(lua_State *L)
{
	component = new Renderable2DComponent();
}


Renderable2DComponent_BindLua::~Renderable2DComponent_BindLua()
{
}

int Renderable2DComponent_BindLua::AddSprite(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetContent() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		wiSprite_BindLua* sprite = Luna<wiSprite_BindLua>::check(L, 2);
		if (sprite != nullptr)
		{
			Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
			if (ccomp != nullptr)
			{
				ccomp->addSprite(sprite->sprite);
			}
			else
			{
				wiLua::SError(L, "AddSprite(Sprite sprite) not a Renderable2DComponent!");
			}
		}
	}
	else
	{
		wiLua::SError(L, "AddSprite(Sprite sprite) not enough arguments!");
	}
	return 0;
}

void Renderable2DComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Renderable2DComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}