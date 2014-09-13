//Talk sprite
player/talk {
	size		10
	shader		sprites/balloon3
	Sprite
}

//Impressive sprite
player/impressive {
	size		10
	shader		medal_impressive
	Sprite
}

//Excellent sprite
player/excellent {
	size		10
	shader		medal_excellent
	Sprite
}

//Holy shit medal from ra3
player/holyshit {
	size		10
	shader		medal_holyshit
	Sprite
}

//Accuracy medal
player/accuracy {
	size		10
	shader		medal_accuracy
	Sprite
}



//Gauntlet sprite
player/gauntlet {
	size		10
	shader		medal_gauntlet
	Sprite
}

//connection sprite
player/connection {
	size		10
	shader		disconnected
	Sprite
}



//haste/speed trail
player/haste {
	interval 0.1 {
		shader		hasteSmokePuff
		emitter 0.5 {
			size	6 + 10 * wave( 0.5  * lerp )
			alphaFade	0
			sprite	cullNear
		}
	}
}

//teleport in effect and sound effect
//input: origin
player/teleportIn {
	sound	sound/world/telein.wav
//Show the fading spawn tube for 0.5 seconds
	model models/misc/telep.md3
	shader teleportEffect
	emitter 0.5 {
		colorFade 0
		anglesModel
	}	
}

//teleport out effect and sound effect
//input: origin
player/teleportOut {
	sound	sound/world/teleout.wav
//Show the fading spawn tube for 0.5 seconds
	model	models/misc/telep.md3
	shader	teleportEffect
	emitter 0.5 {
		colorFade 0
		anglesModel
	}	
}


//Explode the body into gibs
//input: origin, velocity
player/gibbed {
	repeat 11 {
		//Huge list of different body parts that get thrown away 
		modellist loop {
			models/gibs/abdomen.md3
			models/gibs/arm.md3
			models/gibs/chest.md3
			models/gibs/fist.md3
			models/gibs/foot.md3
			models/gibs/forearm.md3
			models/gibs/intestine.md3
			models/gibs/leg.md3
			models/gibs/leg.md3
			models/gibs/skull.md3
			models/gibs/brain.md3
		}

		//aim v0 in random direction, giving more height to upwards vector and scale add to velocity
		random		v0
		v02			v02+1
		addScale	parentVelocity v0 velocity 350
		yaw			360*rand
		pitch		360*rand
		roll		360*rand
		emitter 5 + rand*3 {
			sink 0.9 50
			moveBounce 800 0.6
			anglesModel
			impact 50 {
				size 16 + rand*32
				shader bloodMark
				decal alpha
			}
			distance 20 {
				//Slowly sink downwards
				clear velocity
				velocity2 -10
				rotate rand*360
				shader bloodTrail
				emitter 2 {
					moveGravity 0
					alphaFade 0
					rotate rotate + 10*lerp
					size  8 + 10*lerp
					sprite
				}
			}
		}
	}
}
