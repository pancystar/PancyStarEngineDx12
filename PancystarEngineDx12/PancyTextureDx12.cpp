#include"PancyTextureDx12.h"
using namespace DirectX;
using namespace PancystarEngine;

CommonTextureJsonReflect::CommonTextureJsonReflect()
{

}
void CommonTextureJsonReflect::InitBasicVariable()
{
	Init_Json_Data_Vatriable(reflect_data.heap_type);
	Init_Json_Data_Vatriable(reflect_data.heap_flag_in);
	Init_Json_Data_Vatriable(reflect_data.texture_type);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.Dimension);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.Alignment);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.Width);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.Height);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.DepthOrArraySize);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.MipLevels);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.Format);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.SampleDesc.Count);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.SampleDesc.Quality);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.Layout);
	Init_Json_Data_Vatriable(reflect_data.texture_res_desc.Flags);
	Init_Json_Data_Vatriable(reflect_data.if_gen_mipmap);
	Init_Json_Data_Vatriable(reflect_data.if_force_srgb);
	Init_Json_Data_Vatriable(reflect_data.max_size);
}

uint32_t PancystarEngine::MyCountMips(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
		return 0;

	uint32_t count = 1;
	while (width > 1 || height > 1)
	{
		width >>= 1;
		height >>= 1;
		count++;
	}
	return count;
}
inline bool PancystarEngine::CheckIfDepthStencil(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_D16_UNORM:

#if defined(_XBOX_ONE) && defined(_TITLE)
	case DXGI_FORMAT_D16_UNORM_S8_UINT:
	case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
#endif
		return true;

	default:
		return false;
	}
}
inline void PancystarEngine::MyAdjustPlaneResource(
	_In_ DXGI_FORMAT fmt,
	_In_ size_t height,
	_In_ size_t slicePlane,
	_Inout_ D3D12_SUBRESOURCE_DATA& res)
{
	switch (fmt)
	{
	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:

#if defined(_XBOX_ONE) && defined(_TITLE)
	case DXGI_FORMAT_D16_UNORM_S8_UINT:
	case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
#endif
		if (!slicePlane)
		{
			// Plane 0
			res.SlicePitch = res.RowPitch * height;
		}
		else
		{
			// Plane 1
			res.pData = static_cast<const uint8_t*>(res.pData) + res.RowPitch * height;
			res.SlicePitch = res.RowPitch * ((height + 1) >> 1);
		}
		break;

	case DXGI_FORMAT_NV11:
		if (!slicePlane)
		{
			// Plane 0
			res.SlicePitch = res.RowPitch * height;
		}
		else
		{
			// Plane 1
			res.pData = static_cast<const uint8_t*>(res.pData) + res.RowPitch * height;
			res.RowPitch = (res.RowPitch >> 1);
			res.SlicePitch = res.RowPitch * height;
		}
		break;

	default:
		break;
	}
}
HRESULT PancystarEngine::MyFillInitData(_In_ size_t width,
	_In_ size_t height,
	_In_ size_t depth,
	_In_ size_t mipCount,
	_In_ size_t arraySize,
	_In_ size_t numberOfPlanes,
	_In_ DXGI_FORMAT format,
	_In_ size_t maxsize,
	_In_ size_t bitSize,
	_In_reads_bytes_(bitSize) const uint8_t* bitData,
	_Out_ size_t& twidth,
	_Out_ size_t& theight,
	_Out_ size_t& tdepth,
	_Out_ size_t& skipMip,
	std::vector<D3D12_SUBRESOURCE_DATA>& initData)
{
	if (!bitData)
	{
		return E_POINTER;
	}

	skipMip = 0;
	twidth = 0;
	theight = 0;
	tdepth = 0;

	size_t NumBytes = 0;
	size_t RowBytes = 0;
	const uint8_t* pEndBits = bitData + bitSize;

	initData.clear();

	for (size_t p = 0; p < numberOfPlanes; ++p)
	{
		const uint8_t* pSrcBits = bitData;

		for (size_t j = 0; j < arraySize; j++)
		{
			size_t w = width;
			size_t h = height;
			size_t d = depth;
			for (size_t i = 0; i < mipCount; i++)
			{
				HRESULT hr = DirectX::LoaderHelpers::GetSurfaceInfo(w, h, format, &NumBytes, &RowBytes, nullptr);
				if (FAILED(hr))
					return hr;

				if (NumBytes > UINT32_MAX || RowBytes > UINT32_MAX)
					return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

				if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
				{
					if (!twidth)
					{
						twidth = w;
						theight = h;
						tdepth = d;
					}

					D3D12_SUBRESOURCE_DATA res =
					{
						pSrcBits,
						static_cast<LONG_PTR>(RowBytes),
						static_cast<LONG_PTR>(NumBytes)
					};

					MyAdjustPlaneResource(format, h, p, res);

					initData.emplace_back(res);
				}
				else if (!j)
				{
					// Count number of skipped mipmaps (first item only)
					++skipMip;
				}

				if (pSrcBits + (NumBytes*d) > pEndBits)
				{
					return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
				}

				pSrcBits += NumBytes * d;

				w = w >> 1;
				h = h >> 1;
				d = d >> 1;
				if (w == 0)
				{
					w = 1;
				}
				if (h == 0)
				{
					h = 1;
				}
				if (d == 0)
				{
					d = 1;
				}
			}
		}
	}

	return initData.empty() ? E_FAIL : S_OK;
}
std::string PancyBasicTexture::GetFileTile(const std::string &data_input)
{
	std::string out_pre;
	std::string out_final;
	for (int32_t i = static_cast<int32_t>(data_input.size() - 1); i >= 0; --i)
	{
		if (data_input[i] != '.')
		{
			out_pre += data_input[i];
		}
		else
		{
			break;
		}
	}
	for (int32_t i = static_cast<int32_t>(out_pre.size() - 1); i >= 0; --i)
	{
		out_final += out_pre[i];
	}
	return out_final;
}
//基础纹理
PancyBasicTexture::PancyBasicTexture(const bool &if_could_reload) :PancyCommonVirtualResource<PancyCommonTextureDesc>(if_could_reload)
{
}
PancystarEngine::EngineFailReason PancyBasicTexture::LoadResoureDataByDesc(const PancyCommonTextureDesc &resource_desc)
{
	PancystarEngine::EngineFailReason check_error;
	//如果是从文件中读取的纹理文件则进入加载流程
	if (resource_desc.texture_type == PancyTextureType::Texture_Static_Load)
	{
		//const转非const
		PancyCommonTextureDesc *desc_pointer = const_cast<PancyCommonTextureDesc *>(&resource_desc);
		//启动从图片中加载纹理数据的流程
		check_error = LoadPictureFromFile(*desc_pointer);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
	}
	else
	{
		check_error = BuildEmptyPicture(resource_desc);
		if (!check_error.if_succeed)
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::LoadResoureDataByOtherFile(const std::string &file_name, PancyCommonTextureDesc &resource_desc)
{
	PancyCommonTextureDesc texture_desc;
	texture_desc.texture_type = PancyTextureType::Texture_Static_Load;
	texture_desc.texture_data_file = file_name;
	texture_desc.if_force_srgb = false;
	texture_desc.if_gen_mipmap = false;
	auto check_error = LoadPictureFromFile(texture_desc);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
bool PancyBasicTexture::CheckIfResourceLoadFinish()
{
	PancyResourceLoadState now_load_state = texture_data->GetResourceLoadingState();
	if (now_load_state == PancyResourceLoadState::RESOURCE_LOAD_CPU_FINISH || now_load_state == PancyResourceLoadState::RESOURCE_LOAD_GPU_FINISH)
	{
		return true;
	}
	return false;
}
PancyBasicTexture::~PancyBasicTexture()
{
	if (texture_data != NULL)
	{
		delete texture_data;
	}
}
PancystarEngine::EngineFailReason PancyBasicTexture::LoadPictureFromFile(PancyCommonTextureDesc &new_texture_desc)
{
	PancystarEngine::EngineFailReason check_error;
	std::string picture_path_file = new_texture_desc.texture_data_file;
	//根据路径格式决定是否修改为绝对路径
	//RebuildTextureDataPath(texture_desc.texture_data_file, picture_path_file);

	PancystarEngine::PancyString file_name = picture_path_file;
	std::string file_type = GetFileTile(picture_path_file);
	if (file_type == "dds")
	{
		const DirectX::DDS_HEADER* header = nullptr;
		const uint8_t* bitData = nullptr;
		size_t bitSize = 0;
		std::unique_ptr<uint8_t[]> ddsData;
		//加载dds纹理数据
		HRESULT hr = DirectX::LoaderHelpers::LoadTextureDataFromFile(
			file_name.GetUnicodeString().c_str(),
			ddsData,
			&header,
			&bitData,
			&bitSize
		);
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(hr, "load texture: " + picture_path_file + " error",error_message);
			
			return error_message;
		}
		//检验纹理格式
		UINT width = header->width;
		UINT height = header->height;
		UINT depth = header->depth;
		D3D12_RESOURCE_DIMENSION resDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		UINT arraySize = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		bool isCubeMap = false;
		size_t mipCount = header->mipMapCount;
		if (0 == mipCount)
		{
			mipCount = 1;
		}
		if ((header->ddspf.flags & DDS_FOURCC) &&
			(MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
		{
			auto d3d10ext = reinterpret_cast<const DirectX::DDS_HEADER_DXT10*>(reinterpret_cast<const char*>(header) + sizeof(DirectX::DDS_HEADER));

			arraySize = d3d10ext->arraySize;
			if (arraySize == 0)
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(ERROR_INVALID_DATA, "Texture: " + picture_path_file + " have a wrong size of array",error_message);
				
				return error_message;
			}
			switch (d3d10ext->dxgiFormat)
			{
			case DXGI_FORMAT_AI44:
			case DXGI_FORMAT_IA44:
			case DXGI_FORMAT_P8:
			case DXGI_FORMAT_A8P8:
			{

				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + " ERROR: DDSTextureLoader does not support video textures. Consider using DirectXTex instead.",error_message);
				
				return error_message;
			}
			default:
				if (DirectX::LoaderHelpers::BitsPerPixel(d3d10ext->dxgiFormat) == 0)
				{
					PancystarEngine::EngineFailReason error_message;
					PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Unknown DXGI format (" + std::to_string(static_cast<uint32_t>(d3d10ext->dxgiFormat)) + ")",error_message);
					
					return error_message;
				}
			}

			format = d3d10ext->dxgiFormat;

			switch (d3d10ext->resourceDimension)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				// D3DX writes 1D textures with a fixed Height of 1
				if ((header->flags & DDS_HEIGHT) && height != 1)
				{
					PancystarEngine::EngineFailReason error_message;
					PancyDebugLogError(ERROR_INVALID_DATA, picture_path_file + "D3DX writes 1D textures with a fixed Height of 1",error_message);
					
					return error_message;
				}
				height = depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				if (d3d10ext->miscFlag & 0x4 /* RESOURCE_MISC_TEXTURECUBE */)
				{
					arraySize *= 6;
					isCubeMap = true;
				}
				depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
				{
					PancystarEngine::EngineFailReason error_message;
					PancyDebugLogError(ERROR_INVALID_DATA, picture_path_file + "ERROR: texture3d with volume texture",error_message);
					
					return error_message;
				}

				if (arraySize > 1)
				{
					PancystarEngine::EngineFailReason error_message;
					PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Volume textures are not texture arrays",error_message);
					
					return error_message;
				}
				break;

			default:
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Unknown resource dimension: " + std::to_string(static_cast<uint32_t>(d3d10ext->resourceDimension)),error_message);
				
				return error_message;
			}

			resDim = static_cast<D3D12_RESOURCE_DIMENSION>(d3d10ext->resourceDimension);
		}
		else
		{
			format = DirectX::LoaderHelpers::GetDXGIFormat(header->ddspf);

			if (format == DXGI_FORMAT_UNKNOWN)
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: DDSTextureLoader does not support all legacy DDS formats. Consider using DirectXTex.",error_message);
				
				return error_message;
			}

			if (header->flags & DDS_HEADER_FLAGS_VOLUME)
			{
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
			else
			{
				if (header->caps2 & DDS_CUBEMAP)
				{
					// We require all six faces to be defined
					if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
					{
						PancystarEngine::EngineFailReason error_message;
						PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: DirectX 12 does not support partial cubemaps",error_message);
						
						return error_message;
					}

					arraySize = 6;
					isCubeMap = true;
				}

				depth = 1;
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

				// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
			}

			assert(DirectX::LoaderHelpers::BitsPerPixel(format) != 0);
		}
		if (mipCount > D3D12_REQ_MIP_LEVELS)
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Too many mipmap levels defined for DirectX 12: " + std::to_string(mipCount),error_message);
			
			return error_message;
		}
		switch (resDim)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
				(width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (1D: array " + std::to_string(arraySize) + ", size " + std::to_string(width) + ")\n",error_message);
				
				return error_message;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (isCubeMap)
			{
				// This is the right bound because we set arraySize to (NumCubes*6) above
				if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
					(width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
					(height > D3D12_REQ_TEXTURECUBE_DIMENSION))
				{
					PancystarEngine::EngineFailReason error_message;
					PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (2D cubemap: " + std::to_string(arraySize) + ", size " + std::to_string(width) + "by " + std::to_string(height) + "%u)\n",error_message);
					
					return error_message;
				}
			}
			else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
				(width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
				(height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (2D: " + std::to_string(arraySize) + ", size " + std::to_string(width) + "by " + std::to_string(height) + "%u)\n",error_message);
				
				return error_message;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			if ((arraySize > 1) ||
				(width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
			{
				PancystarEngine::EngineFailReason error_message;
				PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (3D: array " + std::to_string(arraySize) + ", size " + std::to_string(width) + " by " + std::to_string(height) + " by " + std::to_string(depth) + ")\n",error_message);
				
				return error_message;
			}
			break;

		default:
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Unknown resource dimension (" + std::to_string(static_cast<uint32_t>(resDim)) + ")\n",error_message);
			
			return error_message;
		}
		UINT numberOfPlanes = D3D12GetFormatPlaneCount(PancyDx12DeviceBasic::GetInstance()->GetD3dDevice(), format);
		if (!numberOfPlanes)
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_INVALIDARG, "load: " + picture_path_file + " error, device don't support specified format: " + std::to_string(format),error_message);
			
			return error_message;
		}
		if ((numberOfPlanes > 1) && CheckIfDepthStencil(format))
		{
			// DirectX 12 uses planes for stencil, DirectX 11 does not
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(ERROR_NOT_SUPPORTED, "load: " + picture_path_file + " error, DirectX 12 uses planes for stencil, DirectX 11 does not",error_message);
			
			return error_message;
		}
		if_cube_map = isCubeMap;
		//填充纹理数据
		size_t numberOfResources = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
			? 1 : arraySize;
		numberOfResources *= mipCount;
		numberOfResources *= numberOfPlanes;
		if (numberOfResources > D3D12_REQ_SUBRESOURCES)
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_INVALIDARG, "load: " + picture_path_file + " error, texture3d can't afford number of resource: " + std::to_string(numberOfResources),error_message);
			
			return error_message;
		}
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		subresources.reserve(numberOfResources);
		size_t skipMip = 0;
		size_t twidth = 0;
		size_t theight = 0;
		size_t tdepth = 0;
		hr = MyFillInitData(width, height, depth, mipCount, arraySize,
			numberOfPlanes, format,
			new_texture_desc.max_size, bitSize, bitData,
			twidth, theight, tdepth, skipMip, subresources);
		//获取创建格式
		DirectX::DDS_LOADER_FLAGS loadFlags;
		if (new_texture_desc.if_gen_mipmap)
		{
			loadFlags = DirectX::DDS_LOADER_MIP_AUTOGEN;
		}
		else
		{
			loadFlags = DirectX::DDS_LOADER_DEFAULT;
		}
		if (new_texture_desc.if_force_srgb)
		{
			loadFlags = static_cast<DDS_LOADER_FLAGS>(loadFlags | DirectX::DDS_LOADER_FORCE_SRGB);
		}
		if (SUCCEEDED(hr))
		{
			size_t reservedMips = mipCount;
			if (loadFlags & (DirectX::DDS_LOADER_MIP_AUTOGEN | DirectX::DDS_LOADER_MIP_RESERVE))
			{
				reservedMips = std::min<size_t>(D3D12_REQ_MIP_LEVELS, MyCountMips(width, height));
			}
			//根据读取到的纹理信息，重建纹理格式数据
			check_error = BuildTextureResource(resDim, twidth, theight, tdepth, reservedMips - skipMip, static_cast<size_t>(arraySize),
				format, D3D12_RESOURCE_FLAG_NONE, loadFlags, static_cast<int32_t>(subresources.size()), new_texture_desc);
			//修改纹理大小限制重新加载
			if (!check_error.if_succeed)
			{
				if (!new_texture_desc.max_size && (mipCount > 1))
				{
					subresources.clear();

					auto new_max_size = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
						? D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION
						: D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;

					hr = MyFillInitData(width, height, depth, mipCount, arraySize,
						numberOfPlanes, format,
						new_max_size, bitSize, bitData,
						twidth, theight, tdepth, skipMip, subresources);
					if (SUCCEEDED(hr))
					{
						check_error = BuildTextureResource(resDim, twidth, theight, tdepth, mipCount - skipMip, static_cast<size_t>(arraySize),
							format, D3D12_RESOURCE_FLAG_NONE, loadFlags, static_cast<int32_t>(subresources.size()), new_texture_desc);
						if (!check_error.if_succeed)
						{
							PancystarEngine::EngineFailReason error_message;
							PancyDebugLogError(E_INVALIDARG, "load: " + picture_path_file + "error, second try create resource",error_message);
							
							return error_message;
						}
					}
					else
					{
						PancystarEngine::EngineFailReason error_message;
						PancyDebugLogError(E_INVALIDARG, "load: " + picture_path_file + "error, second try fillInitData failed",error_message);
						
						return error_message;
					}
				}
				else
				{
					return check_error;
				}
			}
			check_error = UpdateTextureResource(subresources, new_texture_desc);
			if (!check_error.if_succeed)
			{
				return check_error;
			}
		}
		else
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_INVALIDARG, "load: " + picture_path_file + "error, first try fillInitData failed",error_message);
			
			return error_message;
		}
	}
	else
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(ERROR_INVALID_DATA, "unsupported texture type:" + picture_path_file,error_message);
		
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::UpdateTextureResource(std::vector<D3D12_SUBRESOURCE_DATA> &subresources, const PancyCommonTextureDesc &texture_desc)
{
	PancystarEngine::EngineFailReason check_error;
	//先对待拷贝的资源进行组织
	D3D12_SUBRESOURCE_DATA *subres = &subresources[0];
	UINT subres_size = static_cast<UINT>(subresources.size());
	//获取用于拷贝的commond list
	PancyRenderCommandList *copy_render_list;
	PancyThreadIdGPU copy_render_list_ID;
	check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	//先将数据从内存拷贝到上传缓冲区
	UINT64 RequiredSize = 0;
	UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * subres_size;
	if (MemToAlloc > SIZE_MAX)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "subresource size is bigger than max_size",error_message);
		
		return error_message;
	}
	void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
	if (pMem == NULL)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "alloc heap for write resource failed",error_message);
		
		return error_message;
	}
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
	UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + subres_size);
	UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + subres_size);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&texture_desc.texture_res_desc, 0, subres_size, 0, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
	//拷贝资源数据
	check_error = PancyDynamicRingBuffer::GetInstance()->CopyDataToGpu(copy_render_list, subresources, pLayouts, pRowSizesInBytes, pNumRows, RequiredSize, *texture_data);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	copy_render_list->UnlockPrepare();
	//提交渲染命令
	ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SubmitRenderlist(1, &copy_render_list_ID);
	//分配等待眼位
	PancyFenceIdGPU WaitFence;
	ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(WaitFence);
	check_error = texture_data->SetResourceCopyBrokenFence(WaitFence);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	HeapFree(GetProcessHeap(), 0, pMem);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::BuildTextureResource(
	const D3D12_RESOURCE_DIMENSION &resDim,
	const size_t &width,
	const size_t &height,
	const size_t &depth,
	const size_t &mipCount,
	const size_t &arraySize,
	DXGI_FORMAT &format,
	const D3D12_RESOURCE_FLAGS &resFlags,
	const unsigned int &loadFlags,
	const int32_t &subresources_num,
	PancyCommonTextureDesc &new_texture_desc
)
{
	ComPtr<ID3D12Resource> resource_data;
	PancystarEngine::EngineFailReason check_error;
	if (loadFlags & DirectX::DDS_LOADER_FORCE_SRGB)
	{
		format = DirectX::LoaderHelpers::MakeSRGB(format);
	}
	new_texture_desc.texture_res_desc.Width = static_cast<UINT>(width);
	new_texture_desc.texture_res_desc.Height = static_cast<UINT>(height);
	new_texture_desc.texture_res_desc.MipLevels = static_cast<UINT16>(mipCount);
	new_texture_desc.texture_res_desc.DepthOrArraySize = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? static_cast<UINT16>(depth) : static_cast<UINT16>(arraySize);
	new_texture_desc.texture_res_desc.Format = format;
	new_texture_desc.texture_res_desc.Flags = resFlags;
	new_texture_desc.texture_res_desc.SampleDesc.Count = 1;
	new_texture_desc.texture_res_desc.SampleDesc.Quality = 0;
	new_texture_desc.texture_res_desc.Dimension = resDim;
	new_texture_desc.heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	new_texture_desc.heap_flag_in = D3D12_HEAP_FLAG_NONE;
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(new_texture_desc.heap_type),
		new_texture_desc.heap_flag_in,
		&new_texture_desc.texture_res_desc,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&resource_data)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "Create texture resource error",error_message);
		
		return error_message;
	}
	//计算缓冲区的大小，创建资源块
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&new_texture_desc.texture_res_desc, 0, subresources_num, 0, nullptr, nullptr, nullptr, &subresources_size);
	texture_data = new ResourceBlockGpu(subresources_size, resource_data, new_texture_desc.heap_type, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	//确定加载纹理的SRV格式
	tex_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tex_srv_desc.Format = new_texture_desc.texture_res_desc.Format;
	if (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
	{
		tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
	}
	else if (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		if (!if_cube_map)
		{
			tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		}
		else
		{
			tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		}
	}
	else if (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	}
	tex_srv_desc.Texture2D.MipLevels = new_texture_desc.texture_res_desc.MipLevels;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::BuildEmptyPicture(const PancyCommonTextureDesc &texture_desc)
{
	PancystarEngine::EngineFailReason check_error;
	ComPtr<ID3D12Resource> resource_data;
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = texture_desc.texture_res_desc.Format;
	optClear.Color[0] = 0.0f;
	optClear.Color[1] = 0.0f;
	optClear.Color[2] = 0.0f;
	optClear.Color[3] = 1.0f;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(texture_desc.heap_type),
		texture_desc.heap_flag_in,
		&texture_desc.texture_res_desc,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(&resource_data)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "Create texture resource error",error_message);
		
		return error_message;
	}
	//计算缓冲区的大小
	UINT numberOfPlanes = D3D12GetFormatPlaneCount(PancyDx12DeviceBasic::GetInstance()->GetD3dDevice(), texture_desc.texture_res_desc.Format);
	if (!numberOfPlanes)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_INVALIDARG, "BuildEmptyPicture error, device don't support specified format: " + std::to_string(texture_desc.texture_res_desc.Format),error_message);
		
		return error_message;
	}
	size_t numberOfResources = (texture_desc.texture_res_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		? 1 : texture_desc.texture_res_desc.DepthOrArraySize;
	numberOfResources *= texture_desc.texture_res_desc.MipLevels;
	numberOfResources *= numberOfPlanes;
	if (numberOfResources > D3D12_REQ_SUBRESOURCES)
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_INVALIDARG, "BuildEmptyPicture error, texture3d can't afford number of resource: " + std::to_string(numberOfResources),error_message);
		
		return error_message;
	}
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&texture_desc.texture_res_desc, 0, static_cast<UINT>(numberOfResources), 0, nullptr, nullptr, nullptr, &subresources_size);
	//创建资源块结构
	texture_data = new ResourceBlockGpu(subresources_size, resource_data, texture_desc.heap_type, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	tex_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tex_srv_desc.Format = texture_desc.texture_res_desc.Format;
	tex_srv_desc.Texture2D.MipLevels = 1;
	if (texture_desc.texture_res_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
	{
		tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
	}
	else if (texture_desc.texture_res_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	}
	else if (texture_desc.texture_res_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	}
	//空纹理不需要拷贝操作
	return PancystarEngine::succeed;
}
void PancyBasicTexture::GetJsonFilePath(const std::string &json_file_name, std::string &file_path_out)
{
	file_path_out = "";
	auto copy_size = json_file_name.size();
	for (auto i = json_file_name.size() - 1; i >= 0; --i)
	{
		if (json_file_name[i] != '\\' && json_file_name[i] != '/')
		{
			copy_size -= 1;
		}
		else
		{
			break;
		}
	}
	file_path_out = json_file_name.substr(0, copy_size);
}
void PancyBasicTexture::RebuildTextureDataPath(const std::string &json_file_name, std::string &tex_data_file_name)
{
	bool if_change_path = true;
	//先检查纹理数据路径是否为相对路径
	for (int i = 0; i < tex_data_file_name.size(); ++i)
	{
		if (tex_data_file_name[i] == '\\')
		{
			//路径是绝对路径，不做修改
			if_change_path = false;
			break;
		}
	}
	if (if_change_path)
	{
		//路径是相对路径，需要手动添加绝对路径位置
		string path_file;
		GetJsonFilePath(json_file_name, path_file);
		tex_data_file_name = path_file + tex_data_file_name;
	}
}

PancystarEngine::EngineFailReason PancyBasicTexture::CaptureTextureDataToWindows(DirectX::ScratchImage *new_image)
{
	if (!CheckIfResourceLoadFinish())
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "texture not loading finish,could not Capture texture",error_message);
		
		return error_message;
	}
	HRESULT hr = DirectX::CaptureTexture(
		PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect(),
		texture_data->GetResource(),
		if_cube_map,
		*new_image,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(hr, "could not Capture texture to windows desc",error_message);
		
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::SaveTextureToFile(
	ID3D11Device* pDevice,
	const std::string &file_name,
	bool if_automip,
	bool if_compress
)
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;

	DirectX::ScratchImage *new_image = NULL, *mipmap_image = NULL, *compress_image = NULL;
	bool if_mip_gen = false, if_compress_gen = false;
	new_image = new DirectX::ScratchImage();
	//将纹理数据拍摄到图片中
	check_error = CaptureTextureDataToWindows(new_image);
	if (!check_error.if_succeed)
	{
		return check_error;
	}

	PancyCommonTextureDesc &resource_desc = GetResourceDesc();
	D3D12_RESOURCE_DESC texture_desc = resource_desc.texture_res_desc;
	//为纹理创建mipmap
	if (if_automip && texture_desc.MipLevels == 1)
	{
		auto mipmap_level = MyCountMips(static_cast<uint32_t>(texture_desc.Width), texture_desc.Height);
		mipmap_image = new DirectX::ScratchImage();
		DirectX::GenerateMipMaps(*new_image->GetImages(), TEX_FILTER_DEFAULT | TEX_FILTER_FORCE_NON_WIC, mipmap_level, *mipmap_image);
		if_mip_gen = true;
	}
	else
	{
		mipmap_image = new_image;
	}
	//为纹理创建压缩格式(等microsoft更新/手动使用dx11版本)
	if (if_compress)
	{
		compress_image = new DirectX::ScratchImage();
		DXGI_FORMAT compress_type;
		if (texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || texture_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB || texture_desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)
		{
			compress_type = DXGI_FORMAT_BC7_UNORM_SRGB;
		}
		else if (texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM || texture_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || texture_desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM)
		{
			compress_type = DXGI_FORMAT_BC7_UNORM;
		}
		else
		{
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "compress format could not recognize: " + std::to_string(texture_desc.Format),error_message);
			
			return error_message;
		}
		hr = DirectX::Compress(pDevice,
			mipmap_image->GetImages(),
			mipmap_image->GetImageCount(),
			mipmap_image->GetMetadata(),
			compress_type,
			TEXTURE_COMPRESS_BC7,
			1,
			*compress_image
		);
		if_compress_gen = true;
	}
	else
	{
		compress_image = mipmap_image;
	}
	PancystarEngine::PancyString file_name_sacii(file_name);
	DirectX::SaveToDDSFile(compress_image->GetImages(), compress_image->GetImageCount(), compress_image->GetMetadata(), DDS_FLAGS_NONE, file_name_sacii.GetUnicodeString().c_str());
	delete new_image;
	if (if_mip_gen)
	{
		delete mipmap_image;
	}
	if (if_compress_gen)
	{
		delete compress_image;
	}
	return PancystarEngine::succeed;
}
ResourceBlockGpu* PancystarEngine::GetTextureResourceData(VirtualResourcePointer & virtual_pointer, PancystarEngine::EngineFailReason & check_error)
{
	check_error = PancystarEngine::succeed;
	auto now_texture_resource_value = virtual_pointer.GetResourceData();
	if (now_texture_resource_value->GetResourceTypeName() != typeid(PancyBasicTexture).name())
	{
		PancystarEngine::EngineFailReason error_message;
		PancyDebugLogError(E_FAIL, "the vertex resource is not a texture",error_message);
		
		check_error = error_message;
	}
	const PancyBasicTexture* texture_real_pointer = dynamic_cast<const PancyBasicTexture*>(now_texture_resource_value);
	auto gpu_texture_data = texture_real_pointer->GetGpuResourceData();
	if (gpu_texture_data != NULL)
	{
		return gpu_texture_data;
	}
	return NULL;
}
PancystarEngine::EngineFailReason PancystarEngine::LoadDDSTextureResource(
	const std::string &name_resource_in,
	VirtualResourcePointer &id_need
)
{
	auto check_error = PancyGlobelResourceControl::GetInstance()->LoadResource<PancyBasicTexture>(
		name_resource_in,
		id_need,
		false
		);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancystarEngine::BuildTextureResource(
	const std::string &name_resource_in,
	PancyCommonTextureDesc &resource_data,
	VirtualResourcePointer &id_need,
	bool if_allow_repeat
)
{
	auto check_error = PancyGlobelResourceControl::GetInstance()->LoadResource<PancyBasicTexture>(
		name_resource_in,
		&resource_data,
		typeid(PancyCommonTextureDesc*).name(),
		sizeof(PancyCommonTextureDesc),
		id_need,
		if_allow_repeat
		);
	if (!check_error.if_succeed)
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void PancystarEngine::InitTextureJsonReflect()
{
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_TYPE_DEFAULT);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_TYPE_UPLOAD);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_TYPE_READBACK);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_TYPE_CUSTOM);

	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_SHARED);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_DENY_BUFFERS);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_DISPLAY);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_HARDWARE_PROTECTED);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES);

	JSON_REFLECT_INIT_ENUM(Texture_Static_Load);
	JSON_REFLECT_INIT_ENUM(Texture_Render_Target);
	InitJsonReflectParseClass(PancyCommonTextureDesc, CommonTextureJsonReflect);
}