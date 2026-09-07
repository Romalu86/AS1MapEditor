#include "mapedit/runtime.hpp"
int MENU::Control(INPUT* input)
{
    sprite=0;
    clickFlags&=~3u;

    for (int i=0;i<No();++i) {
        SPRITE* item=*Item(i);
        if (!item || !item->Vid()->m_unknown18 || item->Animation()==14 ||
            item->IsDying() || item->Animation()==7 || item->Animation()==6)
            continue;

        if (item->IsInside(input->mouseX,input->mouseY)) {
            if (!sprite || sprite->Z()<item->Z())
                sprite=item;
        } else {
            item->ChangeAnimation(item->Animation()&1 ? 1 : 0);
        }
    }

    if (!sprite)
        return 0;

    if (input->lClick) {
        clickFlags|=1u;
        input->ClearLClick();
        sprite->ChangeAnimation((sprite->Animation()&1 ? 1 : 0)+4);
        return 1;
    }

    if (input->rClick) {
        clickFlags|=2u;
        input->ClearRClick();
    }

    if ((sprite->Animation()&~1)!=4)
        sprite->ChangeAnimation((sprite->Animation()&1 ? 1 : 0)+2);
    return 0;
}
