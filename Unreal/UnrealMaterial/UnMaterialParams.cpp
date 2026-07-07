// Data-only material parameter extraction for headless builds.
// These implementations are extracted from UnRenderer.cpp (which is #if RENDERING guarded).
// When RENDERING is undefined (macOS headless), UnRenderer.cpp compiles to empty,
// so we provide these here to enable ExportMaterial without OpenGL/SDL.
#if !RENDERING

#include "Core.h"
#include "UnCore.h"
#include "UnObject.h"
#include "UnrealMaterial/UnMaterial.h"
#include "UnrealMaterial/UnMaterial2.h"
#include "UnrealMaterial/UnMaterial3.h"

void UUnrealMaterial::AppendReferencedTextures(TArray<UUnrealMaterial*>& OutTextures, bool onlyRendered) const
{
	guard(UUnrealMaterial::AppendReferencedTextures);

	CMaterialParams Params;
	GetParams(Params);
	Params.AppendAllTextures(OutTextures);

	unguard;
}

void UTexture::GetParams(CMaterialParams &Params) const
{
	Params.Diffuse = (UUnrealMaterial*)this;
}

void UCubemap::GetParams(CMaterialParams &Params) const
{
	Params.Cube = (UUnrealMaterial*)this;
}

void UModifier::GetParams(CMaterialParams &Params) const
{
	guard(UModifier::GetParams);
	if (Material)
		Material->GetParams(Params);
	unguard;
}

void UShader::GetParams(CMaterialParams &Params) const
{
	guard(UShader::GetParams);

	if (Diffuse)
	{
		Diffuse->GetParams(Params);
	}
#if BIOSHOCK
	if (NormalMap)
	{
		if ((UShader*)this == NormalMap) return;
		CMaterialParams Params2;
		NormalMap->GetParams(Params2);
		Params.Normal = Params2.Diffuse;
	}
#endif
	if (SpecularityMask)
	{
		CMaterialParams Params2;
		SpecularityMask->GetParams(Params2);
		Params.Specular          = Params2.Diffuse;
		Params.SpecularFromAlpha = true;
	}
	if (Opacity)
	{
		CMaterialParams Params2;
		Opacity->GetParams(Params2);
		Params.Opacity          = Params2.Diffuse;
		Params.OpacityFromAlpha = true;
	}

	unguardf("%s", Name);
}

#if BIOSHOCK

void UFacingShader::GetParams(CMaterialParams &Params) const
{
	guard(UFacingShader::GetParams);

	if (FacingDiffuse)
	{
		CMaterialParams Params2;
		FacingDiffuse->GetParams(Params2);
		Params.Diffuse = Params2.Diffuse;
	}
	if (NormalMap)
	{
		CMaterialParams Params2;
		NormalMap->GetParams(Params2);
		Params.Normal = Params2.Diffuse;
	}
	if (FacingSpecularColorMap)
	{
		CMaterialParams Params2;
		FacingSpecularColorMap->GetParams(Params2);
		Params.Specular = Params2.Diffuse;
	}
	if (FacingEmissive)
	{
		CMaterialParams Params2;
		FacingEmissive->GetParams(Params2);
		Params.Emissive = Params2.Diffuse;
	}

	unguard;
}

#endif // BIOSHOCK

#if SPLINTER_CELL

void UUnreal3Material::GetParams(CMaterialParams &Params) const
{
	guard(UUnreal3Material::GetParams);

	for (int i = 0; i < ARRAY_COUNT(Textures); i++)
	{
		UTexture *Tex = Textures[i];
		if (!Tex) continue;
		const char *Name = Tex->Name;
		int len = strlen(Name);
		if (!stricmp(Name + len - 2, "_d"))
			Params.Diffuse = Tex;
		else if (!stricmp(Name + len - 2, "_n"))
			Params.Normal = Tex;
		else if (!stricmp(Name + len - 2, "_m"))
			Params.Mask = Tex;
	}
	Params.SpecularMaskChannel = TC_G;

	unguard;
}

void USCX_basic_material::GetParams(CMaterialParams &Params) const
{
	guard(USCX_basic_material::GetParams);

	Params.Diffuse = Base;
	Params.Normal  = Normal;
	Params.Mask    = SpecularMask;
	Params.Cube    = Environment;

	switch (SpecularSource)
	{
	case SpecSrc_SRed:
		Params.SpecularMaskChannel = TC_R;
		break;
	case SpecSrc_SGreen:
		Params.SpecularMaskChannel = TC_G;
		break;
	case SpecSrc_SBlue:
		Params.SpecularMaskChannel = TC_B;
		break;
	case SpecSrc_NAlpha:
		Params.SpecularMaskChannel = TC_MA;
		break;
	}

	unguard;
}

#endif // SPLINTER_CELL

void UCombiner::GetParams(CMaterialParams &Params) const
{
	guard(UCombiner::GetParams);

	CMaterialParams Params2;

	switch (CombineOperation)
	{
	case CO_Use_Color_From_Material1:
		if (Material1) Material1->GetParams(Params2);
		break;
	case CO_Use_Color_From_Material2:
		if (Material2) Material2->GetParams(Params2);
		break;
	case CO_Multiply:
	case CO_Add:
	case CO_Subtract:
	case CO_AlphaBlend_With_Mask:
	case CO_Add_With_Mask_Modulation:
		if (Material1 && Material2)
		{
			if (Material2->IsA("TexEnvMap"))
			{
				Material1->GetParams(Params2);
				Params.Specular = Params2.Diffuse;
				Params.SpecularFromAlpha = true;
			}
			else if (Material1->IsA("TexEnvMap"))
			{
				Material2->GetParams(Params2);
				Params.Specular = Params2.Diffuse;
				Params.SpecularFromAlpha = true;
			}
			else
			{
				Material2->GetParams(Params2);
				if (!Params2.Diffuse) Material1->GetParams(Params2);
			}
		}
		else if (Material1)
			Material1->GetParams(Params2);
		else if (Material2)
			Material2->GetParams(Params2);
		break;
	case CO_Use_Color_From_Mask:
		if (Mask) Mask->GetParams(Params2);
		break;
	}
	Params.Diffuse = Params2.Diffuse;

	unguard;
}

#if UNREAL3

void UMaterialInterface::GetParams(CMaterialParams &Params) const
{
#if SUPPORT_IPHONE
	if (FlattenedTexture)		Params.Diffuse = FlattenedTexture;
	if (MobileBaseTexture)		Params.Diffuse = MobileBaseTexture;
	if (MobileNormalTexture)	Params.Normal  = MobileNormalTexture;
	if (MobileMaskTexture)		Params.Opacity = MobileMaskTexture;
	Params.bUseMobileSpecular  = bUseMobileSpecular;
	Params.MobileSpecularPower = MobileSpecularPower;
	Params.MobileSpecularMask  = MobileSpecularMask;
#endif
}

void UMaterial3::GetParams(CMaterialParams &Params) const
{
	guard(UMaterial3::GetParams);

	Super::GetParams(Params);

	int DiffWeight = 0, NormWeight = 0, SpecWeight = 0, SpecPowWeight = 0, OpWeight = 0, EmWeight = 0, CubeWeight = 0;
	int MaskWeight = 0, EmcWeight = 0;
#define DIFFUSE(check,weight)			\
	if (weight > DiffWeight && check)	\
	{									\
		Params.Diffuse = Tex;			\
		DiffWeight = weight;			\
	}
#define NORMAL(check,weight)			\
	if (weight > NormWeight && check)	\
	{									\
		Params.Normal = Tex;			\
		NormWeight = weight;			\
	}
#define SPECULAR(check,weight)			\
	if (weight > SpecWeight && check)	\
	{									\
		Params.Specular = Tex;			\
		SpecWeight = weight;			\
	}
#define SPECPOW(check,weight)			\
	if (weight > SpecPowWeight && check)\
	{									\
		Params.SpecPower = Tex;		\
		SpecPowWeight = weight;		\
	}
#define OPACITY(check,weight)			\
	if (weight > OpWeight && check)		\
	{									\
		Params.Opacity = Tex;			\
		OpWeight = weight;				\
	}
#define EMISSIVE(check,weight)			\
	if (weight > EmWeight && check)		\
	{									\
		Params.Emissive = Tex;			\
		EmWeight = weight;				\
	}
#define CUBEMAP(check,weight)			\
	if (weight > CubeWeight && check)	\
	{									\
		Params.Cube = Tex;				\
		CubeWeight = weight;			\
	}
#define BAKEDMASK(check,weight)			\
	if (weight > MaskWeight && check)	\
	{									\
		Params.Mask = Tex;				\
		MaskWeight = weight;			\
	}
#define EMISSIVE_COLOR(check,weight)	\
	if (weight > EmcWeight && check)	\
	{									\
		Params.EmissiveColor = Color;	\
		EmcWeight = weight;				\
	}

	int ArGame = GetGame();

	for (int i = 0; i < ReferencedTextures.Num(); i++)
	{
		UTexture3 *Tex = ReferencedTextures[i];
		if (!Tex) continue;
		char Name[256];
		appStrncpylwr(Name, Tex->Name, ARRAY_COUNT(Name));
		int len = strlen(Name);
		if (strstr(Name, "noise")) continue;
		if (strstr(Name, "detail")) continue;

		DIFFUSE(strstr(Name, "diff"), 100);
		NORMAL (strstr(Name, "norm"), 100);
		DIFFUSE(!strcmp(Name + len - 4, "_tex"), 80);
		DIFFUSE(strstr(Name, "_tex"), 60);
		DIFFUSE(!strcmp(Name + len - 2, "_d"), 20);
		OPACITY(strstr(Name, "_om"), 20);
		DIFFUSE (strstr(Name, "_di"), 20);
		DIFFUSE (strstr(Name, "_d" ), 11);
		DIFFUSE (strstr(Name, "albedo"), 19);
		DIFFUSE (!strcmp(Name + len - 2, "_c"), 10);
		DIFFUSE (!strcmp(Name + len - 3, "_cm"), 12);
		NORMAL  (!strcmp(Name + len - 2, "_n"), 20);
		NORMAL  (!strcmp(Name + len - 3, "_nm"), 20);
		NORMAL  (strstr(Name, "_n"), 9);
#if BULLETSTORM
		if (ArGame == GAME_Bulletstorm)
		{
			DIFFUSE (strstr(Name, "_c"), 12);
			NORMAL(strstr(Name, "_ts"), 5);
			SPECULAR(strstr(Name, "_s"), 5);
		}
#endif // BULLETSTORM
		SPECULAR(!strcmp(Name + len - 2, "_s"), 20);
		SPECULAR(strstr(Name, "_s_"), 15);
		SPECPOW (!strcmp(Name + len - 3, "_sp"), 20);
		SPECPOW (!strcmp(Name + len - 3, "_sm"), 20);
		SPECPOW (strstr(Name, "_sp"), 9);
		EMISSIVE(!strcmp(Name + len - 2, "_e"), 20);
		EMISSIVE(!strcmp(Name + len - 3, "_em"), 21);
		OPACITY (!strcmp(Name + len - 2, "_a"), 20);
		if (bIsMasked)
		{
			OPACITY (!strcmp(Name + len - 5, "_mask"), 2);
		}
		DIFFUSE (!strncmp(Name, "df_", 3), 20);
		SPECULAR(!strncmp(Name, "sp_", 3), 20);
		NORMAL  (!strncmp(Name, "no_", 3), 20);

		NORMAL  (strstr(Name, "norm"), 80);
		EMISSIVE(strstr(Name, "emis"), 80);
		SPECULAR(strstr(Name, "specular"), 80);
		OPACITY (strstr(Name, "opac"),  80);
		OPACITY (strstr(Name, "alpha"), 100);

		DIFFUSE(i == 0, 1);
	}
	if ( (Params.Diffuse == Params.Normal && DiffWeight < NormWeight) ||
		 (Params.Diffuse && Params.Diffuse->IsTextureCube()) )
		Params.Diffuse = NULL;

#if UNREAL4
	if (ArGame >= GAME_UE4_BASE)
	{
		Params.PBRMaterial = true;
		if (Params.Specular)
		{
			Params.SpecPower = Params.Specular;
			Params.Specular = NULL;
		}
	}
#endif

	unguard;
}

void UMaterial3::AppendReferencedTextures(TArray<UUnrealMaterial*>& OutTextures, bool onlyRendered) const
{
	guard(UMaterial3::AppendReferencedTextures);
	if (onlyRendered)
	{
		Super::AppendReferencedTextures(OutTextures, onlyRendered);
	}
	else
	{
		for (int i = 0; i < ReferencedTextures.Num(); i++)
		{
			if (ReferencedTextures[i])
				OutTextures.AddUnique(ReferencedTextures[i]);
		}
	}
	unguard;
}

void UTexture2D::GetParams(CMaterialParams &Params) const
{
	Params.Diffuse = (UUnrealMaterial*)this;
}

void UTextureCube3::GetParams(CMaterialParams &Params) const
{
	Params.Cube = (UUnrealMaterial*)this;
}

#if UNREAL4

void UTextureCube4::GetParams(CMaterialParams &Params) const
{
	if (NumSlices != 6) return;
	Params.Cube = (UUnrealMaterial*)this;
}

#endif // UNREAL4

void UMaterialInstanceConstant::GetParams(CMaterialParams &Params) const
{
	guard(UMaterialInstanceConstant::GetParams);

	if (Parent && Parent != this) Parent->GetParams(Params);

	Super::GetParams(Params);
	CMaterialParams ParemtParams = Params;

	int DiffWeight = 0, NormWeight = 0, SpecWeight = 0, SpecPowWeight = 0, OpWeight = 0, EmWeight = 0, EmcWeight = 0, CubeWeight = 0, MaskWeight = 0;

	if (TextureParameterValues.Num())
		Params.Opacity = NULL;

	int ArGame = GetGame();

	int i;
	for (i = 0; i < TextureParameterValues.Num(); i++)
	{
		const FTextureParameterValue &P = TextureParameterValues[i];
		char Name[256];
		appStrncpylwr(Name, P.GetName(), ARRAY_COUNT(Name));
		UTexture3  *Tex  = P.ParameterValue;
		if (!Tex) continue;

		if (strstr(Name, "detail")) continue;
		if (strstr(Name, "gradient")) continue;

		DIFFUSE (strstr(Name, "dif"), 100);
		DIFFUSE (strstr(Name, "albedo"), 100);
		DIFFUSE (strstr(Name, "color"), 80);
		NORMAL  (strstr(Name, "norm") && !strstr(Name, "fx"), 100);
		SPECPOW (strstr(Name, "specpow"), 100);
		SPECULAR(strstr(Name, "spec"), 100);
		EMISSIVE(strstr(Name, "emiss"), 100);
		CUBEMAP (strstr(Name, "cube"), 100);
		CUBEMAP (strstr(Name, "refl"), 90);
		OPACITY (strstr(Name, "opac"), 90);
		OPACITY (strstr(Name, "trans") && !strstr(Name, "transm"), 80);
		OPACITY (strstr(Name, "opacity"), 100);
		OPACITY (strstr(Name, "alpha"), 100);
#if TRON
		if (ArGame == GAME_Tron)
		{
			SPECPOW(strstr(Name, "sppw"), 100);
			EMISSIVE(strstr(Name, "emss"), 100);
			BAKEDMASK(strstr(Name, "mask"), 100);
		}
#endif
#if BATMAN
		if (ArGame == GAME_Batman2)
		{
			BAKEDMASK(!strcmp(Name, "material_attributes"), 100);
			EMISSIVE (strstr(Name, "reflection_mask"), 100);
		}
#endif
#if BLADENSOUL
		if (ArGame == GAME_BladeNSoul)
		{
			BAKEDMASK(!strcmp(Name, "body_mask_rgb"), 100);
		}
#endif
#if DISHONORED
		if (ArGame == GAME_Dishonored)
		{
			CUBEMAP (strstr(Name, "cubemap_tex"), 100);
			EMISSIVE(strstr(Name, "cubemap_mask"), 100);
		}
#endif
#if UNREAL4
		if (ArGame >= GAME_UE4_BASE)
		{
			Params.PBRMaterial = true;
			if (Params.Specular)
			{
				Params.SpecPower = Params.Specular;
				Params.Specular = NULL;
			}
			if (ParemtParams.Emissive == Params.Emissive &&
				ParemtParams.Diffuse != Params.Diffuse)
			{
				Params.Emissive = NULL;
			}
		}
#endif
	}
	for (i = 0; i < VectorParameterValues.Num(); i++)
	{
		const FVectorParameterValue &P = VectorParameterValues[i];
		const char *Name = P.GetName();
		const FLinearColor &Color = P.ParameterValue;
		EMISSIVE_COLOR(strstr(Name, "emissive"), 100);
#if TRON
		if (ArGame == GAME_Tron)
		{
			EMISSIVE_COLOR(strstr(Name, "pipingcolour"), 90);
		}
#endif
	}

#if TRON
	if (ArGame == GAME_Tron)
	{
		if (Params.Mask && Params.SpecPower && Params.Emissive)
			Params.Mask = NULL;
		if (Params.Mask)
		{
			Params.EmissiveChannel      = TC_MA;
			Params.SpecularMaskChannel  = TC_G;
			Params.SpecularPowerChannel = TC_B;
			Params.CubemapMaskChannel   = TC_R;
		}
	}
#endif

#if BATMAN
	if (ArGame == GAME_Batman2)
	{
		if (Params.Mask)
		{
			Params.SpecularMaskChannel  = TC_R;
			Params.SpecularPowerChannel = TC_G;
		}
	}
#endif

#if BLADENSOUL
	if (ArGame == GAME_BladeNSoul)
	{
		if (Params.Mask)
		{
			Params.CubemapMaskChannel   = TC_B;
			Params.SpecularPowerChannel = TC_G;
		}
	}
#endif

	if (!Params.Diffuse && TextureParameterValues.Num() == 1)
		Params.Diffuse = TextureParameterValues[0].ParameterValue;

	unguard;
}

void UMaterialInstanceConstant::AppendReferencedTextures(TArray<UUnrealMaterial*>& OutTextures, bool onlyRendered) const
{
	guard(UMaterialInstanceConstant::AppendReferencedTextures);
	if (onlyRendered)
	{
		Super::AppendReferencedTextures(OutTextures, onlyRendered);
	}
	else
	{
		for (int i = 0; i < TextureParameterValues.Num(); i++)
		{
			if (TextureParameterValues[i].ParameterValue)
				OutTextures.AddUnique(TextureParameterValues[i].ParameterValue);
		}
		if (Parent && Parent != this) Parent->AppendReferencedTextures(OutTextures, onlyRendered);
	}
	unguard;
}

#endif // UNREAL3

#endif // !RENDERING
