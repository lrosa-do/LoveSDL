
 function love.load()
    love.graphics.setBackgroundColor(0, 138, 225)
  end

  function love.keypressed(key)
    if key == "Escape" then
      os.exit()
    end
  end

  local function drawText(str, y)
    local screenw = love.graphics.getWidth()
    local font = love.graphics.getFont()
    love.graphics.print(str, (screenw - font:getWidth(str)) / 2, y)
  end

function drawRotatedRectangle(mode, x, y, width, height, angle)
	local cosa, sina = math.cos(angle), math.sin(angle)

	local dx1, dy1 = width*cosa,   width*sina
	local dx2, dy2 = -height*sina, height*cosa

	local px1, py1 = x,         y
	local px2, py2 = x+dx1,     y+dy1
	local px3, py3 = x+dx1+dx2, y+dy1+dy2
	local px4, py4 = x+dx2,     y+dy2
	
    love.graphics.polygon(mode, px1,py1, px2,py2, px3,py3, px4,py4)
end
  function love.draw()
    love.graphics.setColor(255, 255, 255)
    drawText("LoveDOS " .. love.getVersion(), 90)
    drawText("No game", 102)
    drawText(love.system.getMemUsage(), 152)
    love.graphics.setColor(95, 181, 255)
    drawText("Press ESCAPE to quit", 186)
    
   love.graphics.setColor(255,255,255)
    --local vertices  = {100,100, 200,100, 200,200, 300,200, 300,300, 100,300} 
    --love.graphics.bezier(4,vertices)
    
    drawRotatedRectangle("line",200,200,100,100,90)
    
    local vertices = {100,100, 200,100, 150,200}
   love.graphics.polygon("fill", vertices)
    
  end

