#include <Engine/Resources/FrameIO.hpp>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace entt;

namespace Morpheus {

	template <class Archive>
	void serialize(Archive& archive, ArchiveBlobPointer& m) {
		archive(m.mBegin);
		archive(m.mSize);
	}

	template <class Archive>
	void serialize(Archive& archive, SubFrameResourceTableEntry& m) {
		archive(m.mFrameId);
		archive(CEREAL_NVP(m.mPathString));	
	}

	void FrameHeader::Write(std::ostream& stream) {
		cereal::PortableBinaryOutputArchive out(stream);
		out(mVersion);
		out(CEREAL_NVP(mComponentDirectory));
		out(CEREAL_NVP(mResourceEntries));
		out(CEREAL_NVP(mSubFrames));
		out(CEREAL_NVP(mMaterialsById));

		
	}

	FrameHeader FrameHeader::Read(std::istream& stream) {
		FrameHeader header;
		
		cereal::PortableBinaryInputArchive in(stream);
		in(header.mVersion);
		in(CEREAL_NVP(header.mComponentDirectory));
		in(CEREAL_NVP(header.mResourceEntries));
		in(CEREAL_NVP(header.mSubFrames));
		in(CEREAL_NVP(header.mMaterialsById));

		return header;
	}

	void BuildSerializationSetRecursive(Frame* frame, entt::entity e, SerializationSet* out) {
		for (auto child = frame->GetFirstChild(e); 
			child != entt::null; 
			child = frame->GetNext(child)) {

			if (frame->Has<SubFrameComponent>(e)) {
				out->mSubFrames.emplace_back(child);
			} else {
				out->mToSerialize.emplace_back(child);
				BuildSerializationSetRecursive(frame, child, out);
			}
		}
	}

	SerializationSet BuildSerializationSet(Frame* frame) {
		SerializationSet set;
		BuildSerializationSetRecursive(frame, frame->GetRoot(), &set);
		set.mToSerialize.emplace_back(frame->GetRoot());
		return set;
	}

	void FrameHeader::AddSubFrame(Handle<Frame> frame) {
		auto it = mSubFrames.find(frame->GetFrameId());

		if (it != mSubFrames.end())
			return;

		auto parentPath = mPath.parent_path();

		SubFrameResourceTableEntry entry;
		entry.mFrameId = frame->GetFrameId();
		entry.mPathString = std::filesystem::relative(
			frame->GetPath(), 
			parentPath);

		mSubFrames.emplace(entry);
	}

	void FrameHeader::AddMaterial(Material mat) {
		auto it = mMaterialsById.find(mat.Id());

		if (it == mMaterialsById.end())
			return;

		mMaterialsById[mat.Id()] = mat.GetDesc();
	}

	void FrameHeader::Add(Handle<IResource> resource) {
		auto it = mResourceHandles.find(resource);

		if (it != mResourceHandles.end())
			return;

		auto resourceId = mResourceEntries.size();

		mResourceHandles.emplace_hint(it, resource);
		ResourceTableEntry entry;
		entry.mId = resourceId;
		entry.mResource = resource;

		if (resource->GetFrameId() == mFrameId) {
			entry.mType = ResourceTableEntryType::INTERNAL;
		} else {
			entry.mType = ResourceTableEntryType::EXTERNAL;
		}

		mResourceEntries.emplace_back(entry);

		mIdsByResource[resource] = resourceId;
	}

	ResourceId FrameHeader::GetResourceId(Handle<IResource> resource) {
		return mIdsByResource[resource];
	}

	void FrameHeader::WriteResourceContent(const std::filesystem::path& workingPath, 
		std::ostream& stream) {

		for (auto& resource : mResourceEntries) {

			if (resource.mType == ResourceTableEntryType::INTERNAL) {
				resource.mResource->BinarySerialize(stream);
			}
		}

	}

	void FrameIO::Save(Frame* frame,
		const std::filesystem::path& workingPath,
		std::ostream& stream, 
		const std::vector<SerializableComponentType>& componentTypes) {

		SerializationSet set = BuildSerializationSet(frame);

		// Build the resource table
		// First, add subframes to resource table
		FrameHeader header;
		for (auto& subFrame : set.mSubFrames) {
			auto& subFrameComponent = frame->Registry().get<SubFrameComponent>(subFrame);
			header.AddSubFrame(subFrameComponent.mFrame);
		}

		// Then let all component types build the resource table
		for (auto& type : componentTypes) {
			type->BuildResourceTable(&frame->Registry(), header, set);
		}

		auto headerStartPos = stream.tellp();

		// Write incorrect cache header, come back and fix later
		header.Write(stream);

		auto headerEndPos = stream.tellp();

		// Write internal resource content
		header.WriteResourceContent(workingPath, stream);

		// Serialize components
		for (auto& type : componentTypes) {

			auto componentStart = stream.tellp();
			type->Serialize(&frame->Registry(), stream, header, set);
			auto componentEnd = stream.tellp();

			std::string _name(type->GetType().info().name());
			header.mComponentDirectory[_name] =
				ArchiveBlobPointer{componentStart, componentEnd - componentStart};
		}

		// Reserialize the header
		stream.seekp(headerStartPos);
		header.Write(stream);

		// Make sure that header size has not changed!
		if (stream.tellp() != headerEndPos) {
			std::cerr << "Writes to frame header are inconsistent!" << std::endl;
			throw std::runtime_error("Writes to frame header are inconsistent!");
		}
	}
	
	Task FrameIO::Load(Frame* frame, 
		std::istream& stream, 
		const IResourceCacheCollection* caches,
		const std::vector<SerializableComponentType>& componentTypes) {

		FrameHeader header = FrameHeader::Read(stream);

		auto cacheSet = caches->GetAllCaches();
	}
}