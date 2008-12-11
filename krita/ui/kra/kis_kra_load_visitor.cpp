/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kra/kis_kra_load_visitor.h"
#include "kis_kra_tags.h"
#include "flake/kis_shape_layer.h"

#include <QRect>

#include <KoColorSpaceRegistry.h>
#include <colorprofiles/KoIccColorProfile.h>
#include <KoStore.h>
#include <KoColorSpace.h>

// kritaimage
#include <kis_types.h>
#include <kis_node_visitor.h>
#include <kis_image.h>
#include <kis_selection.h>
#include <kis_layer.h>
#include <kis_transparency_mask.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_adjustment_layer.h>
#include <filter/kis_filter_configuration.h>
#include <kis_datamanager.h>
#include <generator/kis_generator_layer.h>
#include <kis_pixel_selection.h>
#include <kis_clone_layer.h>
#include <kis_filter_mask.h>
#include <kis_transparency_mask.h>
#include <kis_transformation_mask.h>
#include <kis_selection_mask.h>

using namespace KRA;

KisKraLoadVisitor::KisKraLoadVisitor( KisImageSP img,
                                      KoStore *store,
                                      QMap<KisNode *, QString> &layerFilenames,
                                      const QString & name,
                                      int syntaxVersion) :
    KisNodeVisitor(),
    m_layerFilenames(layerFilenames)
{
    m_external = false;
    m_img = img;
    m_store = store;
    m_name = name;
    m_syntaxVersion = syntaxVersion;
}

void KisKraLoadVisitor::setExternalUri(const QString &uri)
{
    m_external = true;
    m_uri = uri;
}

bool KisKraLoadVisitor::visit(KisExternalLayer * layer)
{
    bool result = false;
    if ( KisShapeLayer* shapeLayer = dynamic_cast<KisShapeLayer*>( layer ) ) {
        result =  shapeLayer->loadLayer( m_store, getLocation( layer, DOT_SHAPE_LAYER ) );
    }
    return result;
}

bool KisKraLoadVisitor::visit(KisPaintLayer *layer)
{

    if ( !loadPaintDevice( layer->paintDevice(), getLocation( layer ) ) ) {
        return false;
    }
    if ( !loadProfile( layer->paintDevice(), getLocation( layer, DOT_ICC ) ) ) {
        return false;
    }


    if ( m_syntaxVersion == 1 ) {
        // Check whether there is a file with a .mask extension in the

        // layer directory, if so, it's an old-style transparency mask
        // that should be converted.
        QString location = getLocation( layer, ".mask" );

        if ( m_store->open( location ) ) {

            KisSelectionSP selection = KisSelectionSP(new KisSelection());
            KisPixelSelectionSP pixelSelection = selection->getOrCreatePixelSelection();
            if (!pixelSelection->read(m_store)) {
                pixelSelection->disconnect();
            }
            else {
                KisTransparencyMask* mask = new KisTransparencyMask();
                mask->setSelection( selection );
                m_img->addNode(mask, layer, layer->firstChild());
            }
            m_store->close();
        }
    }
    layer->setDirty(m_img->bounds());
    return true;

}

bool KisKraLoadVisitor::visit(KisGroupLayer *layer)
{
    bool result = visitAll(layer);

    layer->setDirty(m_img->bounds());
    return result;
}

bool KisKraLoadVisitor::visit(KisAdjustmentLayer* layer)
{
    // Adjustmentlayers are tricky: there's the 1.x style and the 2.x
    // style, which has selections with selection components

    if ( m_syntaxVersion == 1 ) {

        //connect(*layer->paintDevice(), SIGNAL(ioProgress(qint8)), m_img, SLOT(slotIOProgress(qint8)));
        QString location = getLocation( layer, ".selection" );
        KisSelectionSP selection = new KisSelection();
        KisPixelSelectionSP pixelSelection = selection->getOrCreatePixelSelection();
        loadPaintDevice( pixelSelection, getLocation( layer, ".selection" ) );
        layer->setSelection(selection);
    }
    else if ( m_syntaxVersion == 2 ) {
        layer->setSelection( loadSelection( getLocation( layer ) ) );
    }
    else {
        // We use the default, empty selection
    }

    loadFilterConfiguration( layer->filter(), getLocation( layer, DOT_FILTERCONFIG ) );

    layer->setDirty(m_img->bounds());
    return true;

}

bool KisKraLoadVisitor::visit(KisGeneratorLayer* layer)
{
    if ( !loadPaintDevice( layer->paintDevice(), getLocation( layer ) ) ) {
        return false;
    }

    if ( !loadProfile( layer->paintDevice(), getLocation( layer, DOT_ICC ) ) ) {
        return false;
    }

    layer->setSelection( loadSelection( getLocation( layer ) ) );

    loadFilterConfiguration( layer->generator(), getLocation( layer, DOT_FILTERCONFIG ) );

    layer->setDirty(m_img->bounds());
    return true;
}


bool KisKraLoadVisitor::visit(KisCloneLayer *layer)
{
    // Clone layers have no data except for their masks
    bool result = visitAll(layer);

    layer->setDirty(m_img->bounds());
    return result;
}

bool KisKraLoadVisitor::visit(KisFilterMask *mask)
{
    mask->setSelection( loadSelection( getLocation( mask ) ) );
    loadFilterConfiguration( mask->filter(), getLocation( mask, DOT_FILTERCONFIG ) );

    mask->setDirty(m_img->bounds());
    return true;
}

bool KisKraLoadVisitor::visit(KisTransparencyMask *mask)
{
    mask->setSelection( loadSelection( getLocation( mask ) ) );
    mask->setDirty(m_img->bounds());

    return true;
}

bool KisKraLoadVisitor::visit(KisTransformationMask *mask)
{

    mask->setSelection( loadSelection( getLocation( mask ) ) );
    mask->setDirty(m_img->bounds());
    return true;
}

bool KisKraLoadVisitor::visit(KisSelectionMask *mask)
{
    mask->setSelection( loadSelection( getLocation( mask ) ) );
    mask->setDirty(m_img->bounds());
    return true;
}

QString KisKraLoadVisitor::getLocation( KisNode* node, const QString& suffix )
{
    QString location = m_external ? QString::null : m_uri;
    location += m_name + "/" + LAYERS + "/" + m_layerFilenames[node] + suffix;

    return location;
}

bool KisKraLoadVisitor::loadPaintDevice( KisPaintDeviceSP device, const QString& location )
{
    //connect(*device, SIGNAL(ioProgress(qint8)), m_img, SLOT(slotIOProgress(qint8)));
    // Layer data
    if (m_store->open(location)) {
        if (!device->read(m_store)) {
            device->disconnect();
            m_store->close();
            //IODone();
            return false;
        }

        m_store->close();
    } else {
        kError() << "No image data: that's an error! " << device << ", " << location;
        return false;
    }
    return true;
}


bool KisKraLoadVisitor::loadProfile( KisPaintDeviceSP device, const QString& location )
{

    if (m_store->hasFile(location)) {
        QByteArray data;
        m_store->open(location);
        data = m_store->read(m_store->size());
        m_store->close();
        // Create a colorspace with the embedded profile
        const KoColorSpace * cs =
            KoColorSpaceRegistry::instance()->colorSpace(device->colorSpace()->id(),
                                                         new KoIccColorProfile(data));
        // replace the old colorspace
        device->setDataManager(device->dataManager(), cs);

    }


}


bool KisKraLoadVisitor::loadFilterConfiguration( KisFilterConfiguration* kfc, const QString& location )
{
    if ( m_store->hasFile(location) ) {
        QByteArray data;
        m_store->open(location);
        data = m_store->read(m_store->size());
        m_store->close();
        if (!data.isEmpty()) {
            kfc->fromLegacyXML(QString(data));
        }
    }
}

KisSelectionSP KisKraLoadVisitor::loadSelection( const QString& location )
{
    KisSelectionSP selection = new KisSelection();

    // Pixel selection
    QString pixelSelectionLocation = location + DOT_PIXEL_SELECTION;
    if ( m_store->hasFile( pixelSelectionLocation ) ) {
        KisPixelSelectionSP pixelSelection = selection->getOrCreatePixelSelection();
        loadPaintDevice( pixelSelection, pixelSelectionLocation );
    }

    // Shape selection
    QString shapeSelectionLocation = location + DOT_SHAPE_SELECTION;
#ifdef __GNUC__
#warning "KisKraLoadVisitor::loadSelection: implement loading of shape selection components."
#endif

    return selection;

}
