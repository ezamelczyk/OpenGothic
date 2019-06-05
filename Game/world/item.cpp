#include "item.h"

#include "game/serialize.h"
#include "worldscript.h"
#include "world.h"

Item::Item(WorldScript &owner, size_t itemInstance)
  :owner(owner) {
  hitem.instanceSymbol = itemInstance;
  hitem.userPtr=this;

  owner.initializeInstance(hitem,itemInstance);
  hitem.amount=1;
  }

Item::Item(WorldScript &owner, Serialize &fin)
  :owner(owner) {
  auto& h = hitem;
  fin.read(h.instanceSymbol);
  fin.read(h.id,h.name,h.nameID,h.hp,h.hp_max,h.mainflag);
  fin.read(h.flags,h.weight,h.value,h.damageType,h.damageTotal,h.damage);
  fin.read(h.wear,h.protection,h.nutrition,h.cond_atr,h.cond_value,h.change_atr,h.change_value,h.magic);
  fin.read(h.on_equip,h.on_unequip,h.on_state);
  fin.read(h.owner,h.ownerGuild,h.disguiseGuild,h.visual,h.visual_change);
  fin.read(h.effect,h.visual_skin,h.scemeName,h.material);
  fin.read(h.munition,h.spell,h.range,h.mag_circle);
  fin.read(h.description,h.text,h.count);
  fin.read(h.inv_zbias,h.inv_rotx,h.inv_roty,h.inv_rotz,h.inv_animate);
  fin.read(h.amount);
  fin.read(pos[0],pos[1],pos[2],equiped,itSlot);
  fin.read(mat);

  view.setObjMatrix(mat);
  }

Item::~Item() {
  }

void Item::save(Serialize &fout) {
  auto& h = hitem;
  fout.write(h.instanceSymbol);
  fout.write(h.id,h.name,h.nameID,h.hp,h.hp_max,h.mainflag);
  fout.write(h.flags,h.weight,h.value,h.damageType,h.damageTotal,h.damage);
  fout.write(h.wear,h.protection,h.nutrition,h.cond_atr,h.cond_value,h.change_atr,h.change_value,h.magic);
  fout.write(h.on_equip,h.on_unequip,h.on_state);
  fout.write(h.owner,h.ownerGuild,h.disguiseGuild,h.visual,h.visual_change);
  fout.write(h.effect,h.visual_skin,h.scemeName,h.material);
  fout.write(h.munition,h.spell,h.range,h.mag_circle);
  fout.write(h.description,h.text,h.count);
  fout.write(h.inv_zbias,h.inv_rotx,h.inv_roty,h.inv_rotz,h.inv_animate);
  fout.write(h.amount);
  fout.write(pos[0],pos[1],pos[2],equiped,itSlot);
  fout.write(mat);
  }

void Item::setView(StaticObjects::Mesh &&m) {
  view = std::move(m);
  view.setObjMatrix(mat);
  }

void Item::clearView() {
  view = StaticObjects::Mesh();
  }

void Item::setPosition(float x, float y, float z) {
  pos={{x,y,z}};
  updateMatrix();
  }

void Item::setDirection(float, float, float) {
  }

void Item::setMatrix(const Tempest::Matrix4x4 &m) {
  mat    = m;
  pos[0] = m.at(3,0);
  pos[1] = m.at(3,1);
  pos[2] = m.at(3,2);
  view.setObjMatrix(m);
  }

const char *Item::displayName() const {
  return hitem.name.c_str();
  }

const char *Item::description() const {
  return hitem.description.c_str();
  }

std::array<float,3> Item::position() const {
  return pos;
  }

bool Item::isGold() const {
  return hitem.instanceSymbol==owner.goldId();
  }

int32_t Item::mainFlag() const {
  return hitem.mainflag;
  }

int32_t Item::itemFlag() const {
  return hitem.flags;
  }

bool Item::isSpellOrRune() const {
  return (uint32_t(mainFlag()) & Inventory::ITM_CAT_RUNE);
  }

bool Item::isSpell() const {
  if(isSpellOrRune())
    return uint32_t(hitem.flags)&Inventory::ITM_MULTI;
  return false;
  }

bool Item::is2H() const {
  auto flg = uint32_t(itemFlag());
  return flg & (Inventory::ITM_2HD_SWD | Inventory::ITM_2HD_AXE);
  }

bool Item::isCrossbow() const {
  auto flg = uint32_t(itemFlag());
  return flg & Inventory::ITM_CROSSBOW;
  }

int32_t Item::spellId() const {
  return hitem.spell;
  }

int32_t Item::swordLength() const {
  return hitem.range;
  }

void Item::setCount(size_t cnt) {
  hitem.amount = cnt;
  }

const char *Item::uiText(size_t id) const {
  return hitem.text[id].c_str();
  }

int32_t Item::uiValue(size_t id) const {
  return hitem.count[id];
  }

size_t Item::count() const {
  return hitem.amount;
  }

int32_t Item::cost() const {
  return hitem.value;
  }

int32_t Item::sellCost() const {
  return int32_t(std::ceil(owner.tradeValueMultiplier()*cost()));
  }

bool Item::checkCond(const Npc &other) const {
  int32_t a=0,v=0;
  return checkCondUse(other,a,v) && checkCondRune(other,a,v);
  }

bool Item::checkCondUse(const Npc &other, int32_t &a, int32_t &nv) const {
  for(size_t i=0;i<Daedalus::GEngineClasses::C_Item::COND_ATR_MAX;++i){
    auto atr = Npc::Attribute(hitem.cond_atr[i]);
    if(other.attribute(atr)<hitem.cond_value[i] && hitem.cond_value[i]!=0) {
      a  = atr;
      nv = hitem.cond_value[i];
      return false;
      }
    }
  return true;
  }

bool Item::checkCondRune(const Npc &other, int32_t &cPl, int32_t &cIt) const {
  cIt = hitem.mag_circle;
  cPl = other.mageCycle();
  return (cPl>=cIt);
  }

size_t Item::clsId() const {
  return hitem.instanceSymbol;
  }

void Item::updateMatrix() {
  mat.identity();
  mat.translate(pos[0],pos[1],pos[2]);
  view.setObjMatrix(mat);
  }
