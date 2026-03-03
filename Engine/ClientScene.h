#pragma once

#include "Scene.h"

class ClientScene : public Scene {

public:
	ClientScene(ServiceRegistry& service) : Scene(service){}
	virtual ~ClientScene() = default;



	void Initialize()			  override;
	void Finalize()				  override;
	void Leave()				  override;
	void FixedUpdate()			  override;
	void Update(float dTime)	  override;
	void StateUpdate(float dTime) override;

};