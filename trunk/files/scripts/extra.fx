
mine/sparkles {
	//Store inverse velocity into v0
	inverse	velocity v0
	normalize v0
	//Slightly increase the y value of v0
	v01	v01 + 0.1
	distance 0.3 {
		wobble	v0 velocity 10 + rand*30
		scale	velocity velocity 100 + rand*150
		size	10 + rand*5
		width	3
		shader	flareShader
		alpha	0.8
		//Pick a random color hue that is somewhat blueish
		colorHue 0.5 + 0.2 *rand
		emitter "0.6 + rand*0.3" {
			moveGravity 200
			colorFade 0.6
			Spark
		}
	}
}

mine/gibbed {
	repeat 200 {
		//aim v0 in random direction, giving more height to upwards vector and scale add to velocity
		random		v0
		v02			v02+1
		addScale	parentVelocity v0 velocity 400
		emitter 0.3 {
			moveGravity 0
			color			1 0 0
			shader		flareshader
			script player/trail
			death {
				repeat 4 {
				   push velocity
					t0 velocity
					normalize velocity
					wobble velocity velocity 30 
					scale velocity velocity t0
					emitter 0.2 {
						moveGravity 00
						color			0 1 0
						shader		flareshader
						script player/trail
						death {
							repeat 4 {
								push velocity
								t0 velocity
								normalize velocity
								wobble velocity velocity 30 
								scale velocity velocity t0
								emitter 0.1 {
									moveGravity 00
									color			0 0 1
									shader		flareshader
									script player/trail
								}
								pop velocity
							}
						}
					}
				   pop velocity
				}
			}
		}
	}
}
