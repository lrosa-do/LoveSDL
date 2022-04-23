
local time = love.timer.getTime or os.time
local last_modified = 0
local fileLoad="main.lua"
local lastscan = 0

  
  
local function handle(err)
  return "Handle ERROR: " .. err
end
function love.boot()

--  package.path = "?.lua;?/init.lua"

  
  
  for i, v in ipairs { love.argv[1], love.argv[2] } do
    local mounted = love.filesystem.mount(v)
    if mounted then
      break
    end
  end


--for key, value in pairs(package.searchers) do
--    print(key, " -- ", value)
--end
  


  -- Init event handlers table
  local function makewrapper(name)
    return function(...)
      if love[name] then return love[name](...) end
    end
  end
  love.handlers = {
    ["quit"] = function() end,
    ["mousepressed"] = makewrapper("mousepressed"),
    ["mousereleased"] = makewrapper("mousereleased"),
    ["mousemoved"] = makewrapper("mousemoved"),
    ["keypressed"] = makewrapper("keypressed"),
    ["keyreleased"] = makewrapper("keyreleased"),
    ["windowresize"] = makewrapper("windowresize"),
   }



  local savedir = "save"
  local mounted = love.filesystem.mount(savedir)
  if mounted then
    love.filesystem.setWriteDir(savedir)
  else
    local old = love.filesystem.write
    love.filesystem.write = function(...)
      love.filesystem.setWriteDir(savedir)
      love.filesystem.mount(savedir)
      love.filesystem.write = old
      return old(...)
    end
  end


  -- Load main.lua or init `nogame` state
  if (love.argv[2]) then
      require(love.argv[2])
      fileLoad = love.argv[2] .. ".lua"
      last_modified =  love.filesystem.getLastModified(fileLoad)
      love.run()
  end
  
  if love.filesystem.isFile("main.lua") then
     liveCode=false
    require("main")
  else
     liveCode=false
    love.nogame()
  end


  love.run()
end


function love.run()

  -- Prepare arguments
  local args = {}
  for i = 2, #love.argv do
    args[i - 1] = love.argv[i]
  end

  -- Do load callback
  if love.load then love.load(args) end
  love.timer.step()



  while true do

    -- Handle events
    love.event.pump()
    while 1 do
      local name, a,b,c,d = love.event.poll()
      
      if not name then
        break
      end
      
    --print(name,a,b,c,d)

      if name == "quit" then
        if not love.quit or not love.quit() then
          os.exit(a)
        end
      end
     if (name) then
        if name ~= "none" then
         --print(name)
         love.handlers[name](a, b, c, d)
        end
     end
    end
    -- Update
    


    love.timer.step()
    local dt = love.timer.getDelta()

    
    
    if love.update then 
        love.update(dt) 
    end
   -- Draw
    love.graphics.clear()
   if love.draw then 
    love.draw() 
   end
   
    if liveCode then
        local diff = time() - lastscan
        if diff > liveCodeInterval then
            lastscan = lastscan + diff
         modified =  love.filesystem.getLastModified(fileLoad)
         if (modified ~= last_modified) then
              last_modified=modified
              if (love.reload) then
               love.reload(fileLoad,liveCodeReloadAssets,dt)
             end
          end
        end
    end
   love.graphics.present()
  end
 
end

function love.reload(file,reset,dt)



        success, chunk = pcall(love.filesystem.load, file)
        if not success then
            print("Fail call " .. tostring(chunk))
            debugoutput = chunk .. "\n"
        end
        ok,err = xpcall(chunk, handle)

        if not ok then 
            print(tostring(err))
            -- Do error main loop
            while true do
            love.event.pump()
            repeat
              local name, a,b,c,d = love.event.poll()
              if name == "keypressed" and b == 40 then
               return 
              end
               if name == "quit" then
                 if not love.quit or not love.quit() then
                       os.exit(a)
                  end
                end
     
            until not name
            love.graphics.clear()
            love.graphics.print(tostring(err), 6, 6)
            love.graphics.present()
            end
            
            if debugoutput then
                debugoutput = (debugoutput .."ERROR: realod ".. err .. "\n" )
            else 
                debugoutput =  err .. "\n" 
            end 
            return false
        else
            debugoutput = nil
        end
        
        if reset then
            loadok, err = xpcall(love.load, handle)
            if not loadok and not loadok_old then
                print("ERROR: "..tostring(err))
                if debugoutput then
                    debugoutput = (debugoutput .."ERROR: ".. err .. "\n" ) 
                else
                    debugoutput =  err .. "\n"
                end
                loadok_old = not loadok
             return false
            end
        end
    updateok, err = pcall(love.update,dt)
    if not updateok and not updateok_old then 
        print("ERROR: "..tostring(err))
        if debugoutput then
            debugoutput = (debugoutput .."ERROR: ".. err .. "\n" ) 
        else
            debugoutput =  err .. "\n"
        end
       return false
  end
  
  updateok_old = not updateok
  return true
end

function love.errhand(msg)
  -- Init error text
  local err = { "Error\n", msg }
  local trace = debug.traceback("", 2)
  for line in string.gmatch(trace, "([^\t]-)\n") do
    table.insert(err, line)
  end
  local str = table.concat(err, "\n")
  while true do
    love.event.pump()
    repeat
      local name, a,b,c,d = love.event.poll()
      if name == "keypressed" and b == 41 then
         return
      end
       if name == "quit" then
                 if not love.quit or not love.quit() then
                       os.exit(a)
                  end
                end
    until not name
    love.graphics.clear()
    love.graphics.print(str, 6, 6)
    print(str)
    love.graphics.present()
  end
end


xpcall(love.boot, love.errhand)
