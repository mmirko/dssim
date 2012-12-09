common_headers='#define NO 0<<<CR>>>#define YES 1<<<CR>>><<<CR>>>'

function 

function transformer(protocol_name)
	opencl_kernel=common_headers
	return opencl_kernel
end

-- Gets the number of registers
function num_registers() 
	if type(registers) ~= 'table' then
		return 0
	end
	num=0
	for k,v in pairs(registers) do num=num+1 end
	return num
end

-- Gets the number of messtypes
function num_messtypes() 
	if type(messtypes) ~= 'table' then
		return 0
	end
	num=0
	for k,v in pairs(messtypes) do num=num+1 end
	return num
end
