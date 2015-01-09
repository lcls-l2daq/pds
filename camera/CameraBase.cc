#include "CameraBase.hh"

Pds::CameraBase::CameraBase() {}

Pds::CameraBase::~CameraBase() {}

int Pds::CameraBase::camera_cfg2() const { return 0; }

static bool _testPattern=false;

void Pds::CameraBase::useTestPattern(bool v) { _testPattern=v; }

bool Pds::CameraBase::_useTestPattern() const { return _testPattern; }
