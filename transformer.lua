common_headers='#define NO 0<<<CR>>>#define YES 1<<<CR>>><<<CR>>>#define EXEC_RESET 0x00<<<CR>>><<<CR>>>#define EXEC_RUN 0x01<<<CR>>>#define EXEC_STATE 0x02<<<CR>>>#define EXEC_MESSAGE 0x04<<<CR>>><<<CR>>>#define RESET_EXEC(x) (x = EXEC_RESET)<<<CR>>><<<CR>>>#define SET_RUN(x) (x |= EXEC_RUN)<<<CR>>>#define SET_STATE(x) (x |= EXEC_STATE)<<<CR>>>#define SET_MESSAGE(x) (x |= EXEC_MESSAGE)<<<CR>>><<<CR>>>'


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
		if #v > 0 then
			result=result..'#define '..k..'_NO 0<<<CR>>>'
			vali=1
			for k1,v1 in ipairs(v) do
				result=result..'#define '..k..'_'..v1..' '..vali..'<<<CR>>>'
				vali=vali+1
			end
		else
			result=result..'#define '..k..'_NO -1<<<CR>>>'
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

	result=result..'\t\t//Processing the execution or not<<<CR>>>'
	result=result..'\t\tif (*(ex_next+nid) <= timetick ) {<<<CR>>>'
	result=result..'\t\t\tSET_RUN(*(ex_stat+nid));<<<CR>>>'
	result=result..'\t\t\t*(ex_next+nid)=timetick+(*(ex_incr+nid));<<<CR>>>'
	result=result..'\t\t} else {<<<CR>>>'
	result=result..'\t\t\tRESET_EXEC(*(ex_stat+nid));<<<CR>>>'
	result=result..'\t\t\treturn;<<<CR>>>'
	result=result..'\t\t}<<<CR>>>'

	return result
end

function makelists()
	result=''
	if (lists) then
		result=result..'<<<CR>>>\t\t//Local lists<<<CR>>>'
		for k,v in pairs(lists) do
--			result=result..'\t\t_local int '..string.lower(k)..'['..v..'];<<<CR>>>'
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

function split(str, pat)
   local t = {}  -- NOTE: use {n = 0} in Lua-5.0
   local fpat = '(.-)' .. pat
   local last_end = 1
   local s, e, cap = str:find(fpat, 1)
   while s do
      if s ~= 1 or cap ~= '' then
	 table.insert(t,cap)
      end
      last_end = e+1
      s, e, cap = str:find(fpat, last_end)
   end
   if last_end <= #str then
      cap = str:sub(last_end)
      table.insert(t, cap)
   end
   return t
end

-- Create a single action for a program
function action_program(mat,act)
	local result=''
	local newact = act

	local iresult
	local itype

	local match1
	local match2
	local match3

	-- Cleaning cr, tabs and multiple space
	newact=string.gsub(newact, '<<<CR>>>+', ' ')
	newact=string.gsub(newact, '\t+', ' ')
	newact=string.gsub(newact, ' +', ' ')
	newact=string.gsub(newact, '^ ', '')
	newact=string.gsub(newact, ' $', '')

	-- start matching

	-- match an integer
	ex,_,match1=string.find(newact, '^(%d+)$')
	if ex~=nil then
		result=result..match1
		return result,'integer'
	end

	-- match an alfadecimal
	ex,_,match1=string.find(newact, '^(%a+)$')
	if ex~=nil then
		for k, _ in pairs(registers) do
			if match1==k then
				result=result..match1
				return result,'register'
			end
		for k, _ in pairs(messtypes) do
			if match1==k then
				result=result..match1
				return result,'messtype'
			end
		end

		if match1=='NEIGHBORS' then
			result=result..match1
			return result,'builtin_list'
		elseif match1=='NOTSENDERS' then
			result=result..match1
			return result,'builtin_list'
		end

	end


-- Temporary return a trad later to be replace by every case
		result=result..match1
		return result,'trad'
	end

	-- match a set operation
	ex,_,match1,match2=string.find(newact, '^set *(%a+) *= *(%a+)$')
	if ex~=nil then
		local regi
		local vali

		regi,itype=action_program(mat,match1)
		if itype==nil then
			return regi,nil
		elseif itype~='register' then
			return match1..' is not a register.',nil
		end

		vali,itype=action_program(mat,match2)
		if itype==nil then
			return vali,nil
		end

		result=result..'\t\t\tstates[nid*registers+REG_'..regi..']='..regi..'_'..vali..';<<<CR>>>'
		return result,'trad'
	end

	-- match a send operation
	ex,_,match1,match2,match3=string.find(newact, '^send *(%a+) *= *(%a+) *to *(%a+)$')
	if ex~=nil then
		local mtype
		local vali
		local dest

		-- Getting the message type and check it
		mtype,itype=action_program(mat,match1)
		if itype==nil then
			return mtype,nil
		elseif itype~='messtype' then
			return match1..' is not a message type.',nil
		end

-- We need to check if vali is a message corresponding to mtype or not
		vali,itype=action_program(mat,match2)
		if itype==nil then
			return vali,nil
		end

		dest,itype=action_program(mat,match3)

		-- Evaluating message destination, it can be a node (integer) or e list or a builtin list
		if itype==nil then
			return dest,nil

		-- Is a built in list
		elseif itype=='builtin_list' then

			if dest=='NEIGHBORS' then
				result=result..'\t\t\tfor (i=0;i<nodes;i++) {<<<CR>>>'
				result=result..'\t\t\t\tif (links[nid*nodes+i] != NO) {<<<CR>>>'
				result=result..'\t\t\t\t\tmessages_out[(nid*nodes+i)*messtypes+MESS_'..mtype..']='..mtype..'_'..vali..';<<<CR>>>'
				result=result..'\t\t\t\t}<<<CR>>>'
				result=result..'\t\t\t}<<<CR>>>'

			elseif dest=='NOTSENDERS' then
				result=result..'\t\t\tfor (i=0;i<nodes;i++) {<<<CR>>>'
				result=result..'\t\t\t\tif (links[nid*nodes+i] != NO) {<<<CR>>>'
				result=result..'\t\t\t\t\tif (messages_in[(i*nodes+nid)*messtypes+MESS_'..mtype..']!='..mtype..'_'..vali..') {<<<CR>>>'
				result=result..'\t\t\t\t\t\tmessages_out[(nid*nodes+i)*messtypes+MESS_'..mtype..']='..mtype..'_'..vali..';<<<CR>>>'
				result=result..'\t\t\t\t\t}<<<CR>>>'
				result=result..'\t\t\t\t}<<<CR>>>'
				result=result..'\t\t\t}<<<CR>>>'
			end

		end

		return result,'trad'
	end

	-- Nothing else matches so trying to split by ;
	for comm in string.gmatch(newact,'([^;]+);') do
		iresult,itype=action_program(mat,comm)
		if itype==nil then
			return iresult,nil
		else
			result=result..iresult
		end
	end

	return result,'trad'
end


-- The entity logic
function create_actions()
	local result=''
	local rresult=''
	local rtype=''

	for k,v in pairs(actions) do
		result=result..'\t\tif ('
		for k1,v1 in ipairs(k) do
			if k1~=1 then result=result..'&&' end
			result=result..'(flag_'..string.lower(v1)..'==YES)'
		end
		result=result..') {<<<CR>>>'
		if type(v) == 'string' then
			rresult,rtype=action_program(k,v)

			if rtype~=nil then
				result=result..rresult
			else
				return rresult,nil
			end
		end
		result=result..'\t\t}<<<CR>>><<<CR>>>'
	end
	return result,'ok'
end


-- The option that go after the entity activity
function footer_options()
	local result=''

	for k,v in ipairs(options) do
		if v == 'RESET_MESS' then
			result=result..'<<<CR>>>\t\t// Clean up all the received messages<<<CR>>>'
			result=result..'\t\tfor (i=0;i<nodes;i++) {<<<CR>>>'
			for k,v in pairs(messtypes) do
				result=result..'\t\t\tif (messages_in[(i*nodes+nid)*messtypes+MESS_'..k..'] != '..k..'_NO) {<<<CR>>>'
				result=result..'\t\t\t\tmessages_in[(i*nodes+nid)*messtypes+MESS_'..k..']='..k..'_NO;<<<CR>>>'
				result=result..'\t\t\t}<<<CR>>>'
			end
			result=result..'\t\t}<<<CR>>>'
		end
	end
	return result
end

function footer()
	local result=''
	result=result..'\t}<<<CR>>>'
	result=result..'}<<<CR>>>'
	return result
end

-- The kernel prototype
function prototype(protocol_name)
	result='__kernel void '..protocol_name..'(\t__global int * ex_stat,<<<CR>>>\t\t\t\t__global int * ex_incr,<<<CR>>>\t\t\t\t__global int * ex_next,<<<CR>>>\t\t\t\t__global int *links,<<<CR>>>\t\t\t\t__global int *states,<<<CR>>>\t\t\t\t__global int *messages_in,<<<CR>>>\t\t\t\t__global int *messages_out,<<<CR>>>\t\t\t\tconst unsigned int messtypes,<<<CR>>>\t\t\t\tconst unsigned int registers,<<<CR>>>\t\t\t\tconst unsigned int nodes,<<<CR>>>\t\t\t\tconst unsigned int timetick)<<<CR>>>'
	return result
end

-- The main lus function, it takes the protocol name and create the kernel data
function transformer(protocol_name)
	local result=''
	local rtype=''

	opencl_kernel=common_headers
	opencl_kernel=opencl_kernel..defines()
	opencl_kernel=opencl_kernel..prototype(protocol_name)
	opencl_kernel=opencl_kernel..header()
	opencl_kernel=opencl_kernel..makelists()

	-- This is the composition of the internal state variables
	opencl_kernel=opencl_kernel..'<<<CR>>>\t\t//Get the entity states<<<CR>>>'
	for k,v in pairs(registers) do
		opencl_kernel=opencl_kernel..'\t\tint register_'..string.lower(k)..' = states[nid*registers+REG_'..k..'];<<<CR>>>'

		for k1,v1 in ipairs(v) do
			opencl_kernel=opencl_kernel..'\t\tint flag_'..string.lower(k)..'_'..string.lower(v1)..' = NO;<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\tif ( register_'..string.lower(k)..' == '..k..'_'..v1..' ) { flag_'..string.lower(k)..'_'..string.lower(v1)..' = YES; }<<<CR>>>'
		end
	end

	-- This is the received messages
	opencl_kernel=opencl_kernel..'<<<CR>>>\t\t//Check the received messages<<<CR>>>'
	for k,v in pairs(messtypes) do
		if #v > 0 then
			for k1,v1 in ipairs(v) do
				opencl_kernel=opencl_kernel..'\t\tint flag_'..string.lower(k)..'_'..string.lower(v1)..' = NO;<<<CR>>>'

				opencl_kernel=opencl_kernel..'\t\tfor (i=0;i<nodes;i++) {<<<CR>>>'
				opencl_kernel=opencl_kernel..'\t\t\tif (messages_in[(i*nodes+nid)*messtypes+MESS_'..k..'] == '..k..'_'..v1..') {<<<CR>>>'
				opencl_kernel=opencl_kernel..'\t\t\t\tflag_'..string.lower(k)..'_'..string.lower(v1)..' = YES;<<<CR>>>'
				opencl_kernel=opencl_kernel..'\t\t\t}<<<CR>>>'
				opencl_kernel=opencl_kernel..'\t\t}<<<CR>>><<<CR>>>'
			end
		else
			opencl_kernel=opencl_kernel..'\t\tint flag_'..string.lower(k)..'_ring = NO;<<<CR>>>'

			opencl_kernel=opencl_kernel..'\t\tfor (i=0;i<nodes;i++) {<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t\tif (messages_in[(i*nodes+nid)*messtypes+MESS_'..k..'] != '..k..'_NO) {<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t\t\tif (messages_in[(i*nodes+nid)*messtypes+MESS_'..k..'] == 0) {<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t\t\t\tflag_'..string.lower(k)..'_ring = YES;<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t\t\t} else {<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t\t\t\tmessages_out[(i*nodes+nid)*messtypes+MESS_'..k..'] = messages_in[(i*nodes+nid)*messtypes+MESS_'..k..']-1;<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t\t\t}<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t\t}<<<CR>>>'
			opencl_kernel=opencl_kernel..'\t\t}<<<CR>>><<<CR>>>'
		end
	end

	-- Cicle all state possibilities
--	starttable={}
--	for k,v in pairs(registers) do
--		table.insert(starttable,k)
--	end
--	for k,v in ipairs(state_machine(starttable)) do
--		for k1,v1 in ipairs(v) do
--			opencl_kernel=opencl_kernel..v1..' '
--		end
--		opencl_kernel=opencl_kernel..'<<<CR>>>'
--	end

	result,rtype=create_actions()

	if rtype~=nil then
		opencl_kernel=opencl_kernel..result
	else
		return result,nil
	end

	opencl_kernel=opencl_kernel..footer_options()
	opencl_kernel=opencl_kernel..footer()
	return opencl_kernel,'ok'
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
		if #v > 0 then
			if k..'_NO'==regormess then
				return 0
			end
			for k1,v1 in ipairs(v) do
				if k..'_'..v1==regormess then
					return k1-1
				end
			end
		else
			if k..'_NO'==regormess then
				return -1
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
				if #v > 0 then
					for k1,v1 in ipairs(v) do
						if cktab==registers then
							if k1-1==lid then
								return k..'_'..v1
							end
						else
							if lid==0 then
								return k..'_NO'
							end
							if k1==lid then
								return k..'_'..v1
							end
						end
					end
				else
					if lid==-1 then
						return k..'_NO'
					elseif lid==0 then
						return k..'_RING'
					else
						return k..'_'..tostring(lid)
					end
				end
			end
		end
		num=num+1
	end
	return nil
end

-- Get the default state for the register reg (index) if there is not such e default the simulation cannot start
function get_default_state(reg)
	if type(registers) ~= 'table' then
		return nil
	end
	if type(defaults) ~= 'table' then
		return nil
	end

	regi=0
	for k,v in pairs(registers) do
		if regi==reg then
			for k1,v1 in ipairs(v) do
				for k2,v2 in ipairs(defaults) do
					if v2==k..'_'..v1 then
						return k1-1
					end
				end
			end		
		end
		regi=regi+1
	end
	return nil
end

-- Get the default state for the messages (index) if there is not such e default the simulation cannot start
function get_default_mess(reg)
	if type(messtypes) ~= 'table' then
		return nil
	end
	if type(defaults) ~= 'table' then
		return nil
	end

	regi=0
	for k,v in pairs(messtypes) do
		if regi==reg then
			if #v > 0 then
				for k1,v1 in ipairs(v) do
					for k2,v2 in ipairs(defaults) do
						if v2==k..'_NO' then
							return 0
						end
						if v2==k..'_'..v1 then
							return k1
						end
					end
				end
			else
				for k2,v2 in ipairs(defaults) do
					if v2==k..'_NO' then
						return -1
					end
				end
			end		
		end
		regi=regi+1
	end
	return nil
end

-- Get the ending state for the register reg (index) if there is not such e default the simulation cannot start
function get_ending_state(reg)
	if type(registers) ~= 'table' then
		return nil
	end
	if type(defaults) ~= 'table' then
		return nil
	end

	regi=0
	for k,v in pairs(registers) do
		if regi==reg then
			for k1,v1 in ipairs(v) do
				for k2,v2 in ipairs(ending) do
					if v2==k..'_'..v1 then
						return k1-1
					end
				end
			end		
		end
		regi=regi+1
	end
	return nil
end

-- Get the ending state for the messages (index) if there is not such e default the simulation cannot start
function get_ending_mess(reg)
	if type(messtypes) ~= 'table' then
		return nil
	end
	if type(defaults) ~= 'table' then
		return nil
	end

	regi=0
	for k,v in pairs(messtypes) do
		if regi==reg then
			if #v > 0 then
				for k1,v1 in ipairs(v) do
					for k2,v2 in ipairs(ending) do
						if v2==k..'_NO' then
							return 0
						end
						if v2==k..'_'..v1 then
							return k1
						end
					end
				end
			else
				for k2,v2 in ipairs(ending) do
					if v2==k..'_NO' then
						return -1
					end
				end
			end		
		end
		regi=regi+1
	end
	return nil
end

-- Get the number of boundary condition elements or nil
function get_boundary_num(step)
	if boundary[step] == nil then
		return nil
	end
	tta=boundary[step]
	num=0
	for k,v in pairs(tta) do num=num+1 end
	return num
end

-- Get the name of the node at index bindex and temporal step step
function get_boundary_el_name(step,bindex)
	if boundary[step] == nil then
		return nil
	end
	tta=boundary[step]
	num=0
	for k,v in pairs(tta) do
		if num==bindex then
			return k
		end
		num=num+1
	end
	return nil
end

-- Get the state of the register reg of the node with the given name and temporal step step
function get_boundary_el_state(step,name,reg)
	if boundary[step] == nil then
		return nil
	end
	tta=boundary[step]
	regi=0
	for k,v in pairs(registers) do
		if regi==reg then
			for k1,v1 in ipairs(v) do
				for k2,v2 in ipairs(tta[name]) do
					if v2==k..'_'..v1 then
						return k1-1
					end
				end
			end		
		end
		regi=regi+1
	end
	return nil
	
end

-- Get the state the spontaneus impulse reg of the node with the given name and temporal step step
function get_boundary_el_mess(step,name,reg)
	if boundary[step] == nil then
		return nil
	end
	tta=boundary[step]
	regi=0
	for k,v in pairs(messtypes) do
		if regi==reg then
			for k1,v1 in ipairs(v) do
				for k2,v2 in ipairs(tta[name]) do
					if v2==k..'_NO' then
						return 0
					end
					if v2==k..'_'..v1 then
						return k1
					end
				end
			end		
		end
		regi=regi+1
	end
	return nil
end

function check_report(report_name)
	if type(report) ~= 'table' then
		return nil
	end
	for _,value in pairs(report) do
		if value==report_name then
			return 1
		end
	end
	return nil
end

-- Restrictions

-- Get the number of restrictions
function get_restrictions_num()
	if type(restrictions) ~= 'table' then
		return 0
	end
	num=0
	for k,v in pairs(restrictions) do num=num+1 end
	return num
end

-- Get the name of a restriction
function get_restriction_name(choosen)
	num=0
	for k,v in pairs(restrictions) do
		if choosen==num then
			return v
		end
		num=num+1
	end
	return num
end


