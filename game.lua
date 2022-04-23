print("Game teste")


local function load(modulename)
  local errmsg = ""
  -- Find source

  local modulepath = string.gsub(modulename, "%.", "/")
  print("Path:",modulepath)

  for path in string.gmatch(package.path, "([^;]+)") do
    print(path)
    local filename = string.gsub(path, "%?", modulepath)
    print(filename)
    local file = io.open(filename, "rb")
    if file then
      print(" Compile and return the module")
      return assert(loadstring(assert(file:read("*a")), filename))
    end
    errmsg = errmsg.."\n\tno file '"..filename.."' (checked with custom loader)"
  end

  return errmsg
end


  package.path = "?.lua;?/init.lua"
  table.insert(package.loaders, 1, function(modname)
    modname = modname:gsub("%.", "/")
    print("mOD NAme:",modname)
    for x in package.path:gmatch("[^;]+") do
    print("x:",x)
      local file = x:gsub("?", modname)
      print(file)
      ---if love.filesystem.exists(file) then
       -- return assert(loadstring(love.filesystem.read(file), "=" .. file))
      --end
    end
  end)

--table.insert(package.loaders, 2, load)
print("Lua assert example is here")
local _variable1 = 12
assert( _variable1 == 12, "the variable is not greater than 12 ")
print( _variable1 )


