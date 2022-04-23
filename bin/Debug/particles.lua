
function love.load()
 
     love.graphics.setBlendMode("add")
     particle = love.graphics.newImage('assets/particle.png')


print(liveCode)
print(liveCodeInterval)


  particles = {}
  for i = 0, 200 do
    table.insert(particles, {
      x = 0,
      y = 0,
      vx = 0,
      vy = 0,
      t = 0,
      a = 0,
      s = math.random(),}
    )
  end 
    
   love.mouse.setVisible(false)
end

function love.update(dt)

 for i, p in ipairs(particles) do
    p.x = p.x + p.vx * dt
    p.y = p.y + p.vy * dt
    if p.t > 0 then
      p.t = p.t - dt
    else
      p.a = p.a - dt * 3
      if p.a < 0 then
        local r = math.random() * math.pi * 2
        p.x = math.cos(r) * 20
        p.y = math.sin(r) * 20
        p.vx = (1 - math.random() * 2) * 90
        p.vy = (1 - math.random() * 2) * 90
        p.t = math.random() * 1
        p.a = 1
      end
    end

  end

  
end

function love.draw()

    love.graphics.Fps(5,5)   

    
      w = love.graphics.getWidth() / 2
      h = love.graphics.getHeight() / 2

     love.graphics.setColor(255, 55, 0)



     love.graphics.setBlendMode("blend")

    love.graphics.setColor(255, 255, 255)
    for i, p in ipairs(particles) do
    love.graphics.setAlpha(p.a*155)
    love.graphics.draw(particle, w + p.x,h + p.y,nil,p.s,p.s)

    love.graphics.setColor(255, 255, 255)
    
    love.graphics.pie('fill',100,100,20,-40,40)
    
    love.graphics.arc(100,200,20,-40,40)
    
--    love.graphics.box(200,200,250,255)
    
        love.graphics.rounded('line',200,200,250,255,30)
    
    




  end


 

    
end

function love.mousepressed( x, y, button,  presses )
   
end
function love.keypressed( key )
  if (key =="Escape") then
    love.event.quit()
  end
end

