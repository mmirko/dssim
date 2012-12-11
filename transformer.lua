common_headers='#define NO 0<<<CR>>>#define YES 1<<<CR>>><<<CR>>>'

-- Generates the kernel defines
function defines()
	result=''
	regi=0
	for k,v in pairs(registers) do
		result=result..'#define REG_'..k..' '..regi..'<<<CR>>><<<CR>>>'
		regi=regi+1
		vali=0
		for k1,v1 in ipairs(v) do
			result=result..'#define '..k..'_'..v1..' '..vali..'<<<CR>>>'
			vali=vali+1
		end
	result=result..'<<<CR>>>'
	end
	regi=0
	for k,v in pairs(messtypes) do
		result=result..'#define MESS_'..k..' '..regi..'<<<CR>>><<<CR>>>'
		regi=regi+1
		result=result..'#define '..k..'_NO 0<<<CR>>>'
		vali=1
		for k1,v1 in ipairs(v) do
			result=result..'#define '..k..'_'..v1..' '..vali..'<<<CR>>>'
			vali=vali+1
		end
	result=result..'<<<CR>>>'
	end
	return result
end

function header()
	result=''
	result=result..'{<<<CR>>>'
	result=result..'\t//Get our global thread ID<<<CR>>>'
	result=result..'\tint nid = get_global_id(0);<<<CR>>><<<CR>>>'

	result=result..'\t// Counters<<<CR>>>'
	result=result..'\tint i,j,k;<<<CR>>><<<CR>>>'

	result=result..'\t//Make sure we do not go out of bounds<<<CR>>>'
	result=result..'\tif (nid < nodes) {<<<CR>>>'

	return result
end

function makelists()
	result=''
	if (lists) then
		result=result..'<<<CR>>>\t\t//Local lists<<<CR>>>'
		for k,v in pairs(lists) do
			result=result..'\t\t_local int '..string.lower(k)..'['..v..'];<<<CR>>>'
			result=result..'\t\tint '..string.lower(k)..'_index=0;<<<CR>>>'
		end
	end
	return result
end

function table.copy(t)
	local t2 = {}
	for k,v in pairs(t) do
		t2[k] = v
	end
	return t2
end

function state_machine(ttemp)
	result={}
	for k,v in ipairs(ttemp) do
		newresult={}
		sv=registers[v]
		for k1,v1 in ipairs(sv) do
			if (# result ~= 0) then
				for k2,v2 in ipairs(result) do
					subre={}
					for k3,v3 in ipairs(v2) do
						table.insert(subre,v3)
					end
					table.insert(subre,v..'_'..v1)
					table.insert(newresult,subre)
				end
			else
				table.insert(newresult,{v..'_'..v1})
			end
		end
		result=newresult
	end

	return result
end

function footer()
	result=''
	result=result..'\t}<<<CR>>>'
	result=result..'}<<<CR>>>'
	return result
end

-- The kernel prototype
function prototype(protocol_name)
	result='__kernel void '..protocol_name..'(\t__global int *links,<<<CR>>>\t\t\t\t__global int *states,<<<CR>>>\t\t\t\t__global int *messages_in,<<<CR>>>\t\t\t\t__global int *messages_out,<<<CR>>>\t\t\t\tconst unsigned int messtypes,<<<CR>>>\t\t\t\tconst unsigned int registers,<<<CR>>>\t\t\t\tconst unsigned int nodes)<<<CR>>>'
	return result
end

-- The main lus function, it takes the protocol name and create the kernel data
function transformer(protocol_name)
	opencl_kernel=common_headers
	opencl_kernel=opencl_kernel..defines()
	opencl_kernel=opencl_kernel..prototype(protocol_name)
	opencl_kernel=opencl_kernel..header()
	opencl_kernel=opencl_kernel..makelists()

	-- This is the composition of the internal state variables
	opencl_kernel=opencl_kernel..'<<<CR>>>\t\t//Get the entity states<<<CR>>>'
	for k,v in pairs(registers) do
		opencl_kernel=opencl_kernel..'\t\tint register_'..string.lower(k)..' = states[nid*registers+REG_'..k..'];<<<CR>>>'
	end

	-- Cicle all state possibilities
	starttable={}
	for k,v in pairs(registers) do
		table.insert(starttable,k)
	end
	for k,v in ipairs(state_machine(starttable)) do
		for k1,v1 in ipairs(v) do
			opencl_kernel=opencl_kernel..v1..' '
		end
		opencl_kernel=opencl_kernel..'<<<CR>>>'
	end

	opencl_kernel=opencl_kernel..footer()
	return opencl_kernel
end

-- Gets the number of registers, it is used to transfer this value to dssim
function num_registers() 
	if type(registers) ~= 'table' then
		return 0
	end
	num=0
	for k,v in pairs(registers) do num=num+1 end
	return num
end

-- Gets the number of messtypes, it is used to transfer this value to dssim
function num_messtypes() 
	if type(messtypes) ~= 'table' then
		return 0
	end
	num=0
	for k,v in pairs(messtypes) do num=num+1 end
	return num
end

-- Get the id of the state or message
function name_to_id(regormess)
	num=0
	for k,v in pairs(registers) do
		if 'REG_'..k==regormess then
			return num
		end
		for k1,v1 in ipairs(v) do
			if k..'_'..v1==regormess then
				return k1-1
			end
		end
	end
	num=0
	for k,v in pairs(messtypes) do
		if 'MESS_'..k==regormess then
			return num
		end
		for k1,v1 in ipairs(v) do
			if k..'_'..v1==regormess then
				return k1-1
			end
		end
	end
	return nil
end

-- Get the name of the state or message from the id: rtype is register or messtype, cid is the class id, lid is the subclass id
function id_to_name(rtype,cid,lid)
	if rtype == 'register' then
		cktab=registers
		prefix='REG_'
	else 
		cktab=messtypes
		prefix='MESS_'
	end
	num=0
	for k,v in pairs(cktab) do
		if num==cid then
			if lid==nil then
				return prefix..k
			else
				for k1,v1 in ipairs(v) do
					if k1-1==lid then
						return k..'_'..v1
					end
				end
			end
		end
		num=num+1
	end
	return nil
end
