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
