#include "Holodeck.h"
#include "Conversion.h"

const bool USE_RHS = true;

FRotator RPYToRotator(float Roll, float Pitch, float Yaw){
	/* Takes Right-Handed Roll, Pitch, Yaw, and creates left-handed rotation
		about X, Y, Z in the fixed frame, or Z, Y, X about current frame. 
		
	I don't trust UE4 with rotations after I found this:
	https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/Core/Public/Math/Rotator.h#L484
	(Combining 2 rotations by adding euler angle's *shiver*)
	*/
	FMatrix R;
	
	float SP, SY, SR;
	float CP, CY, CR;
	FMath::SinCos(&SP, &CP, FMath::DegreesToRadians(Pitch));
	FMath::SinCos(&SY, &CY, FMath::DegreesToRadians(Yaw));
	FMath::SinCos(&SR, &CR, FMath::DegreesToRadians(Roll));

	// Row, then column index
	// First (X) Column
	R.M[0][0] = CY*CP;
	R.M[0][1] = CP*SY;
	R.M[0][2] = -SP;

	// Second (Y) Column
	R.M[1][0] = CY*SP*SR - CR*SY;
	R.M[1][1] = CY*CR + SY*SP*SR;
	R.M[2][1] = CP*SR;

	// Third (Z) Column
	R.M[2][0] = SY*SR + CY*CR*SP;
	R.M[2][1] = CR*SY*SP - CY*SR;
	R.M[2][2] = CP*CR;

	// Flip y-axes in and out to make it left-handed
	R.M[0][1] *= -1;
	R.M[1][0] *= -1;
	R.M[2][1] *= -1;
	R.M[1][2] *= -1;
	
	return R.Rotator();
}

FVector ConvertLinearVector(FVector Vector, ConvertType Type) {

	float ScaleFactor = 1.0;
	if (Type == UEToClient)
		ScaleFactor /= UEUnitsPerMeter;
	else if(Type == ClientToUE)
		ScaleFactor *= UEUnitsPerMeter;

	Vector.X *= ScaleFactor;
	Vector.Y *= ScaleFactor;
	Vector.Z *= ScaleFactor;

	if(USE_RHS)
		Vector.Y *= -1;

	return Vector;
}

FVector ConvertAngularVector(FVector Vector, ConvertType Type) {

	float ScaleFactor = 1.0;
	if (Type == UEToClient)
		ScaleFactor /= UEUnitsPerMeter;
	else if (Type == ClientToUE)
		ScaleFactor *= UEUnitsPerMeter;

	Vector.X *= ScaleFactor;
	Vector.Y *= ScaleFactor;
	Vector.Z *= ScaleFactor;

	if (USE_RHS) {
		Vector.X *= -1;
		Vector.Z *= -1;
	}

	return Vector;
}


FRotator ConvertAngularVector(FRotator Rotator, ConvertType Type) {

	if (USE_RHS) {
		Rotator.Roll *= -1;
		Rotator.Yaw *= -1;
	}

	return Rotator;
}

FVector ConvertTorque(FVector Vector, ConvertType Type) {

	float ScaleFactor = 1.0;
	if (Type == UEToClient)
		ScaleFactor /= UEUnitsPerMeterSquared;
	else if (Type == ClientToUE)
		ScaleFactor *= UEUnitsPerMeterSquared;

	Vector.X *= ScaleFactor;
	Vector.Y *= ScaleFactor;
	Vector.Z *= ScaleFactor;

	if (USE_RHS) {
		Vector.X *= -1;
		Vector.Z *= -1;
	}

	return Vector;
}

float ConvertClientDistanceToUnreal(float client) {
	return client * UEUnitsPerMeter;
}

float ConvertUnrealDistanceToClient(float unreal) {
	return unreal / UEUnitsPerMeter;
}