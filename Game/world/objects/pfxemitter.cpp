#include "pfxemitter.h"

#include "graphics/pfx/pfxobjects.h"
#include "graphics/pfx/pfxbucket.h"
#include "graphics/pfx/particlefx.h"

#include "world/world.h"
#include "utils/fileext.h"

using namespace Tempest;

PfxEmitter::PfxEmitter(PfxBucket& b, size_t id)
  :bucket(&b), id(id) {
  }

PfxEmitter::PfxEmitter(World& world, const std::string& name)
  :PfxEmitter(world,name.c_str()) {
  }

PfxEmitter::PfxEmitter(World& world, const char* name)
  :PfxEmitter(world,world.script().getParticleFx(name)) {
  }

PfxEmitter::PfxEmitter(World& world, const ParticleFx* decl)
  :PfxEmitter(world.view()->pfxGroup,decl) {
  }

PfxEmitter::PfxEmitter(PfxObjects& owner, const ParticleFx* decl) {
  if(decl==nullptr || decl->visMaterial.tex==nullptr)
    return;
  std::lock_guard<std::recursive_mutex> guard(owner.sync);
  bucket = &owner.getBucket(*decl);
  id     = bucket->allocEmitter();
  trail  = TrlObjects::Item(owner.trails,*decl);
  if(decl->shpMesh!=nullptr && decl->shpMeshRender)
    shpMesh = owner.world.addView(decl->shpMesh_S.c_str(),0,0,0);
  }

PfxEmitter::PfxEmitter(World& world, const ZenLoad::zCVobData& vob) {
  auto& owner = world.view()->pfxGroup;
  if(FileExt::hasExt(vob.visual,"PFX")) {
    auto decl = world.script().getParticleFx(vob.visual.c_str());
    if(decl==nullptr || decl->visMaterial.tex==nullptr)
      return;
    std::lock_guard<std::recursive_mutex> guard(owner.sync);
    bucket = &owner.getBucket(*decl);
    id     = bucket->allocEmitter();

    trail = TrlObjects::Item(world,*decl);
    } else {
    Material mat(vob);
    std::lock_guard<std::recursive_mutex> guard(owner.sync);
    bucket = &owner.getBucket(mat,vob);
    id     = bucket->allocEmitter();
    }
  }

PfxEmitter::~PfxEmitter() {
  if(bucket!=nullptr) {
    std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
    bucket->freeEmitter(id);
    }
  }

PfxEmitter::PfxEmitter(PfxEmitter && b)
  :bucket(b.bucket), id(b.id), trail(std::move(b.trail)) {
  b.bucket = nullptr;
  }

PfxEmitter& PfxEmitter::operator=(PfxEmitter &&b) {
  std::swap(bucket,b.bucket);
  std::swap(id,    b.id);
  std::swap(trail, b.trail);
  return *this;
  }

void PfxEmitter::setPosition(float x, float y, float z) {
  setPosition(Vec3(x,y,z));
  }

void PfxEmitter::setPosition(const Vec3& pos) {
  if(bucket==nullptr)
    return;
  std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
  trail.setPosition(pos);
  auto& v = bucket->get(id);
  v.pos = pos;
  if(v.next!=nullptr)
    v.next->setPosition(pos);
  if(v.block==size_t(-1))
    return; // no backup memory
  auto& p = bucket->getBlock(*this);
  p.pos = pos;
  }

void PfxEmitter::setTarget(const Vec3& pos) {
  if(bucket==nullptr)
    return;
  std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
  auto& v = bucket->get(id);
  v.target    = pos;
  v.hasTarget = true;
  }

void PfxEmitter::setDirection(const Matrix4x4& d) {
  if(bucket==nullptr)
    return;
  std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
  auto& v = bucket->get(id);
  v.direction[0] = Vec3(d.at(0,0),d.at(0,1),d.at(0,2));
  v.direction[1] = Vec3(d.at(1,0),d.at(1,1),d.at(1,2));
  v.direction[2] = Vec3(d.at(2,0),d.at(2,1),d.at(2,2));

  if(v.next!=nullptr)
    v.next->setDirection(d);
  }

void PfxEmitter::setActive(bool act) {
  if(bucket==nullptr)
    return;
  std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
  auto& v     = bucket->get(id);
  auto  state = (act ? PfxBucket::S_Active : PfxBucket::S_Inactive);
  if(act==state)
    return;
  v.st = state;
  if(v.next!=nullptr)
    v.next->setActive(act);
  if(act==true)
    v.waitforNext = bucket->decl.ppsCreateEmDelay;
  }

bool PfxEmitter::isActive() const {
  if(bucket==nullptr)
    return false;
  std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
  auto& v = bucket->get(id);
  return v.st==PfxBucket::S_Active;
  }

void PfxEmitter::setLooped(bool loop) {
  if(bucket==nullptr)
    return;
  std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
  auto& v = bucket->get(id);
  v.isLoop = loop;
  if(v.next!=nullptr)
    v.next->setLooped(loop);
  }

void PfxEmitter::setMesh(const MeshObjects::Mesh* mesh, const Pose* pose) {
  const PfxEmitterMesh* m = (mesh!=nullptr) ? mesh->toMeshEmitter() : nullptr;

  std::lock_guard<std::recursive_mutex> guard(bucket->parent.sync);
  auto& v = bucket->get(id);
  v.mesh = m;
  v.pose = pose;
  if(v.next!=nullptr)
    v.next->setMesh(mesh,pose);
  }

uint64_t PfxEmitter::effectPrefferedTime() const {
  if(bucket==nullptr)
    return 0;
  return bucket->decl.effectPrefferedTime();
  }

void PfxEmitter::setObjMatrix(const Matrix4x4 &mt) {
  setPosition (mt.at(3,0),mt.at(3,1),mt.at(3,2));
  setDirection(mt);
  shpMesh.setObjMatrix(mt);
  }
