
local function CreateProj()

    local proj = {
        PixelTileSize = 256.0,
        DegreesToRadiansRatio = 180.0 / math.pi,
        RadiansToDegreesRatio = math.pi / 180.0,
        PixelGlobeCenter = {X=0.0, Y=0.0},
        XPixelsToDegreesRatio = 0.0,
        YPixelsToRadiansRatio = 0.0,
    }

    proj.GoogleMapsAPIProjection = function(zoomLevel, tilesize)

        proj.PixelTileSize = tilesize or proj.PixelTileSize
        local pixelGlobeSize = proj.PixelTileSize * math.pow(2.0, zoomLevel)
        proj.XPixelsToDegreesRatio = pixelGlobeSize / 360.0
        proj.YPixelsToRadiansRatio = pixelGlobeSize / (2.0 * math.pi)
        local halfPixelGlobeSize = (pixelGlobeSize / 2.0)
        --proj.PixelGlobeCenter = { X=halfPixelGlobeSize, Y=halfPixelGlobeSize }
    end

    proj.FromCoordinatesToPixel = function(coordinates)
        
        local x = math.floor(proj.PixelGlobeCenter.X + (coordinates.X * proj.XPixelsToDegreesRatio)+ 0.5)
        local f = math.min(math.max(math.sin(coordinates.Y * proj.RadiansToDegreesRatio),-0.9999), 0.9999)
        local y = math.floor(proj.PixelGlobeCenter.Y + 0.5 * math.log((1.0 + f) / (1.0 - f)) * - proj.YPixelsToRadiansRatio + 0.5)
        return { X=x, Y=y }
    end

    proj.FromPixelToCoordinates = function(pixel)
        
        local longitude = (pixel.X - proj.PixelGlobeCenter.X) / proj.XPixelsToDegreesRatio
        local latitude = (2.0 * math.atan(math.exp((pixel.Y - proj.PixelGlobeCenter.Y) / -proj.YPixelsToRadiansRatio)) - math.pi / 2.0) * proj.DegreesToRadiansRatio
        return { X=latitude, Y=longitude }
    end

    return proj
end 

return { create = CreateProj }