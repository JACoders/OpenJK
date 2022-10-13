/*[Vertex]*/

uniform float u_Time; // delta time
uniform vec2 u_MapZExtents;
uniform vec3 u_EnvForce;
uniform vec4 u_RandomOffset;

in vec3 attr_Position;
in vec3 attr_Color;

out vec3 var_Position;
out vec3 var_Velocity;

const float CHUNK_EXTENDS = 2000.0;
const float HALF_CHUNK_EXTENDS = CHUNK_EXTENDS * 0.5;

vec3 NewParticleZPosition()
{
	vec3 position = var_Position;
	position.xy += u_RandomOffset.xy;
	position.z += u_MapZExtents.y - u_MapZExtents.x;

	vec2 signs = sign(position.xy);
	vec2 absPos = abs(position.xy) + vec2(HALF_CHUNK_EXTENDS);
	position.xy = -signs * (HALF_CHUNK_EXTENDS - mod(absPos, CHUNK_EXTENDS));

	return position;
}

vec3 NewParticleXYPosition(vec3 in_Position)
{
	vec3 position = in_Position;
	position.xy += u_RandomOffset.xy * 10.0;
	vec2 signs = sign(position.xy);
	vec2 absPos = abs(position.xy) + vec2(HALF_CHUNK_EXTENDS);
	position.xy = -signs * (1.5 * CHUNK_EXTENDS - mod(absPos, CHUNK_EXTENDS));

	return position;
}

void main()
{
	var_Velocity = attr_Color;
	var_Velocity = mix(var_Velocity, u_EnvForce, u_Time * 0.002);
	var_Position = attr_Position;
	var_Position += var_Velocity * u_Time;

	if (var_Position.z < u_MapZExtents.x)
	{
		var_Position = NewParticleZPosition();
		var_Velocity.xy = u_EnvForce.xy;
	}
	if (u_EnvForce.z == 0 && any(greaterThan(abs(var_Position).xy, vec2(1.5 * CHUNK_EXTENDS))))
	{
		var_Position = NewParticleXYPosition(var_Position);
		var_Velocity.xy = u_EnvForce.xy;
	}
}
